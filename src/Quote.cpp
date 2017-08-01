/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: Quote.cpp
Version: 1.0
Date: 2017.4.25

History:
shengkaishan     2017.4.25   1.0     Create
******************************************************************************/


#include "Quote.h"
#include "QuoteConfig.h"
//#include <Windows.h>
#include <iostream>
#include <string.h>
#include "common.h"
#include "applog.h"
#include "md_server.h"
#include "future_platform.h"

using namespace std;
namespace future
{

    Quote::Quote(void) :
        m_pAPI(NULL),
        m_requestid(0),
        m_bfront_status(false),
        m_blogin_status(false),
        m_connect_state(false),
        m_chk_thread(nullptr),
        m_running(true)
    {
        m_map_contract.clear();
    }


    Quote::~Quote(void)
    {
        m_map_contract.clear();
    }


    void Quote::SetAPI(CThostFtdcMdApi *pAPI)
    {
        m_pAPI = pAPI;
    }


    int Quote::Run()
    {
        if (NULL == m_pAPI) {
            cerr << "error: m_pAPI is NULL." << endl;
            return -1;
        }

        int ret = TAPIERROR_SUCCEED;
        //���ӷ�����IP���˿�
        QString log_str = "���������������";
        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        emit signals_write_log(log_str);
        string key = "md_info/ip";
        QString ip = common::get_config_value(key).toString();
        key = "md_info/port";
        int port = common::get_config_value(key).toInt();
        string addr;
        addr.append("tcp://").append(ip.toStdString()).append(":").append(std::to_string(port));
        m_pAPI->RegisterSpi(this);                                          // ע���¼���
        m_pAPI->RegisterFront(const_cast<char*>(addr.c_str()));             // ��������ǰ�õ�ַ
        m_pAPI->Init();                                                     // ��������
        //�ȴ�m_bfront_status
        m_Event.WaitEvent();
        if (!m_bfront_status) {
            return ret;
        }

        //��¼������
        log_str = "���ڵ�¼�������";
        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        emit signals_write_log(log_str);
        string key = "md_info/brokerid";
        QString md_brokerid = common::get_config_value(key).toString();
        key = "md_info/userid";
        QString md_userid = common::get_config_value(key).toString();
        key = "md_info/passwd";
        QString md_passwd = common::getXorEncryptDecrypt(
            common::get_config_value(key).toString());
        CThostFtdcReqUserLoginField loginReq;
        memset(&loginReq, 0, sizeof(loginReq));
        strcpy(loginReq.BrokerID, md_brokerid.toStdString.c_str());
        strcpy(loginReq.UserID, md_userid.toStdString().c_str());
        strcpy(loginReq.Password, md_passwd.toStdString().c_str());
        ret = m_pAPI->ReqUserLogin(&loginReq, m_requestid++);
        if (TAPIERROR_SUCCEED != ret) {
            cout << "ReqUserLogin Error:" << ret << endl;
            return ret;
        }

        //�ȴ�m_blogin_status
        m_Event.WaitEvent();
        if (!m_blogin_status) {
            return ret;
        }

        log_str = "��������¼���";
        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        emit signals_write_log(log_str);
        emit signals_quote_reconnect();

        m_connect_state = true;
        return ret;
    }

    void Quote::req_sub_market_data(string& contract)
    {
        QString log_str = QObject::tr("%1%2%3").arg("����").arg(contract.c_str()).arg("����");
        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        emit signals_write_log(log_str);

        int ret = TAPIERROR_SUCCEED;
        //��������
        char ppInstrumentID[1][10];
        strcpy(ppInstrumentID[0], contract.c_str());
        ret = m_pAPI->SubscribeMarketData((char**)ppInstrumentID, 1);
        if (TAPIERROR_SUCCEED != ret) {
            cout << "SubscribeMarketData Error:" << ret << endl;
            return;
        }
    }

    void Quote::req_unsub_market_data(string& contract)
    {
        QString log_str = QObject::tr("%1%2%3").arg("ȡ������").arg(contract.c_str()).arg("����");
        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        emit signals_write_log(log_str);
        int ret = TAPIERROR_SUCCEED;
        //��������
        char ppInstrumentID[1][10];
        strcpy(ppInstrumentID[0], contract.c_str());
        ret = m_pAPI->UnSubscribeMarketData((char**)ppInstrumentID, 1);
        if (TAPIERROR_SUCCEED != ret) {
            cout << "UnSubscribeMarketData Error:" << ret << endl;
            return;
        }
    }

    void Quote::thread_reconnect()
    {
        while (m_running) {
            if (!m_connect_state) {
                Run();
            }
            Sleep(3000);
        }
    }

    // ---- ctp_api�ص����� ---- //
    // ���ӳɹ�Ӧ��
    void Quote::OnFrontConnected()
    {
        QString log_str = "md API���ӳɹ�";
        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        emit signals_write_log(log_str);
        m_bfront_status = true;
        m_Event.SignalEvent();
    }

    // �Ͽ�����֪ͨ
    void Quote::OnFrontDisconnected(int nReason)
    {
        if (!m_running) return;
        QString log_str = QObject::tr("%1%2").arg("API�Ͽ�,�Ͽ�ԭ��:").
            arg(nReason);
        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        emit signals_write_log(log_str);

        m_connect_state = false;
        if (m_chk_thread != nullptr) return;
        m_chk_thread = std::make_shared<std::thread>(
            std::bind(&Quote::thread_reconnect, this));
    }

    // ������ʱ����
    void Quote::OnHeartBeatWarning(int nTimeLapse)
    {
        std::cerr << "=====����������ʱ=====" << std::endl;
        std::cerr << "���ϴ�����ʱ�䣺 " << nTimeLapse << std::endl;
    }

    // ��¼Ӧ��
    void Quote::OnRspUserLogin(
        CThostFtdcRspUserLoginField *pRspUserLogin,
        CThostFtdcRspInfoField *pRspInfo,
        int nRequestID,
        bool bIsLast)
    {
        bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
        if (!bResult) {
            QString log_str = "md API��¼�ɹ�";
            APP_LOG(applog::LOG_INFO) << log_str.toStdString();
            emit signals_write_log(log_str);
            m_blogin_status = true;
        }
        else {
            QString log_str = QObject::tr("%1%2").arg("��¼ʧ�ܣ�������:").
                arg(pRspInfo->ErrorID);
            APP_LOG(applog::LOG_INFO) << log_str.toStdString();
            emit signals_write_log(log_str);
            
        }
        m_Event.SignalEvent();
    }

    // �ǳ�Ӧ��
    void Quote::OnRspUserLogout(
        CThostFtdcUserLogoutField *pUserLogout,
        CThostFtdcRspInfoField *pRspInfo,
        int nRequestID,
        bool bIsLast)
    {
        std::cout << __FUNCTION__ << std::endl;
        bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
        if (!bResult) {
            std::cout << "�˻��ǳ��ɹ�" << std::endl;
            std::cout << "�����̣� " << pUserLogout->BrokerID << std::endl;
            std::cout << "�ʻ����� " << pUserLogout->UserID << std::endl;
        }
        else {
            std::cerr << "���ش���ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
        }
    }

    // ����֪ͨ
    void Quote::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
    {
        std::cout << __FUNCTION__ << std::endl;
        bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
        if (bResult)
            std::cerr << "���ش���ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
    }


    // ��������Ӧ��
    void Quote::OnRspSubMarketData(
        CThostFtdcSpecificInstrumentField *pSpecificInstrument,
        CThostFtdcRspInfoField *pRspInfo,
        int nRequestID,
        bool bIsLast)
    {
        bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
        if (!bResult) {
            QString log_str = "���鶩�ĳɹ�";
            APP_LOG(applog::LOG_INFO) << log_str.toStdString();
            emit signals_write_log(log_str);
        }
        else {
            QString log_str = QObject::tr("%1%2").arg("���鶩��ʧ�ܣ������룺").
                arg(pRspInfo->ErrorID);
            APP_LOG(applog::LOG_INFO) << log_str.toStdString();
            emit signals_write_log(log_str);

        }
    }

    // ȡ����������Ӧ��
    void Quote::OnRspUnSubMarketData(
        CThostFtdcSpecificInstrumentField *pSpecificInstrument,
        CThostFtdcRspInfoField *pRspInfo,
        int nRequestID,
        bool bIsLast)
    {
        std::cout << __FUNCTION__ << std::endl;
        bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
        if (!bResult) {
            std::cout << "=====ȡ����������ɹ�=====" << std::endl;
            std::cout << "��Լ���룺 " << pSpecificInstrument->InstrumentID << std::endl;
        }
        else
            std::cerr << "���ش���ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
    }

    // ��������֪ͨ
    void Quote::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData)
    {
        if (NULL != pDepthMarketData) {
            emit signals_quote_changed(QString::number(pDepthMarketData->LastPrice, 10, 3));
        }
    }
}