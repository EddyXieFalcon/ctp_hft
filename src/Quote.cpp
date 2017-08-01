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
        m_bIsAPIReady(false),
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
            cout << "Error: m_pAPI is NULL." << endl;
            return -1;
        }

        int iErr = 0;
        //�趨������IP���˿�
        string key = "md_info/ip";
        QString ip = common::get_config_value(key).toString();
        key = "md_info/port";
        int port = common::get_config_value(key).toInt();
        string addr;
        addr.append("tcp://").append(ip.toStdString()).append(":").append(std::to_string(port));
        m_pAPI->RegisterSpi(this);                                          // ע���¼���
        m_pAPI->RegisterFront(const_cast<char*>(addr.c_str()));           // ��������ǰ�õ�ַ
        m_pAPI->Init();                                                 // ��������


        //��¼������
        QString log_str = "���ڵ�¼�������";
        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        emit signals_write_log(log_str);
        string key = "md_info/userid";
        QString md_userid = common::get_config_value(key).toString();
        key = "md_info/passwd";
        QString md_passwd = common::getXorEncryptDecrypt(
            common::get_config_value(key).toString());

        TapAPIQuoteLoginAuth stLoginAuth;
        memset(&stLoginAuth, 0, sizeof(stLoginAuth));
        strcpy(stLoginAuth.UserNo, md_userid.toStdString().c_str());
        strcpy(stLoginAuth.Password, md_passwd.toStdString().c_str());
        stLoginAuth.ISModifyPassword = APIYNFLAG_NO;
        stLoginAuth.ISDDA = APIYNFLAG_NO;
        iErr = m_pAPI->Login(&stLoginAuth);
        if (TAPIERROR_SUCCEED != iErr) {
            cout << "Login Error:" << iErr << endl;
            return iErr;
        }

        //�ȴ�APIReady
        m_Event.WaitEvent();
        if (!m_bIsAPIReady) {
            return iErr;
        }

        //�õ�����Ʒ��
        //APP_LOG(applog::LOG_INFO) << "Commodity starting";
        //m_uiSessionID = 0;
        //m_pAPI->QryCommodity(&m_uiSessionID);
        ////�ȴ�QryCommodity
        //m_Event.WaitEvent();
        //if (!m_bCommodity) {
        //    return;
        //}

        //�õ����к�Լ
        log_str = "���ڻ�ȡ����������ݡ�����";
        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        emit signals_write_log(log_str);

        m_uiSessionID = 0;
        TapAPICommodity com;
        memset(&com, 0, sizeof(com));
        m_pAPI->QryContract(&m_uiSessionID, &com);
        //�ȴ�QryContract
        m_Event.WaitEvent();
        if (!m_bContract) {
            return iErr;
        }

        log_str = "��������¼���";
        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        emit signals_write_log(log_str);
        emit signals_quote_reconnect();

        m_connect_state = true;
        return iErr;
    }

    void Quote::req_sub_market_data(string& contract)
    {
        QString log_str = QObject::tr("%1%2%3").arg("����").arg(contract.c_str()).arg("����");
        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        emit signals_write_log(log_str);

        TAPIINT32 iErr = TAPIERROR_SUCCEED;
        //��������
        int i = 0;
        for (i = 0; i < contract.length(); i++) {
            if (contract[i] > '0' && contract[i] < '9') {
                break;
            }
        }
        string commodity_no = contract.substr(0, i);
        string contract_no = contract.substr(i, contract.length() - i);
        TapAPIContract stContract; //CL1705
        memset(&stContract, 0, sizeof(stContract));
        strcpy(stContract.Commodity.ExchangeNo, m_map_contract[contract].c_str());
        stContract.Commodity.CommodityType = DEFAULT_COMMODITY_TYPE;
        strcpy(stContract.Commodity.CommodityNo, commodity_no.c_str());
        strcpy(stContract.ContractNo1, contract_no.c_str());
        stContract.CallOrPutFlag1 = TAPI_CALLPUT_FLAG_PUT;
        stContract.CallOrPutFlag2 = TAPI_CALLPUT_FLAG_PUT;
        m_uiSessionID = 0;
        iErr = m_pAPI->SubscribeQuote(&m_uiSessionID, &stContract);
        if (TAPIERROR_SUCCEED != iErr) {
            cout << "SubscribeQuote Error:" << iErr << endl;
            return;
        }
    }

    void Quote::req_unsub_market_data(string& contract)
    {
        QString log_str = QObject::tr("%1%2%3").arg("ȡ������").arg(contract.c_str()).arg("����");
        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        emit signals_write_log(log_str);
        TAPIINT32 iErr = TAPIERROR_SUCCEED;
        //��������
        int i = 0;
        for (i = 0; i < contract.length(); i++) {
            if (contract[i] > '0' && contract[i] < '9') {
                break;
            }
        }
        string commodity_no = contract.substr(0, i);
        string contract_no = contract.substr(i, contract.length() - i);
        TapAPIContract stContract; //CL1705
        memset(&stContract, 0, sizeof(stContract));
        strcpy(stContract.Commodity.ExchangeNo, m_map_contract[contract].c_str());
        stContract.Commodity.CommodityType = DEFAULT_COMMODITY_TYPE;
        strcpy(stContract.Commodity.CommodityNo, commodity_no.c_str());
        strcpy(stContract.ContractNo1, contract_no.c_str());
        stContract.CallOrPutFlag1 = TAPI_CALLPUT_FLAG_NONE;
        stContract.CallOrPutFlag2 = TAPI_CALLPUT_FLAG_NONE;
        m_uiSessionID = 0;
        iErr = m_pAPI->UnSubscribeQuote(&m_uiSessionID, &stContract);
        if (TAPIERROR_SUCCEED != iErr) {
            cout << "UnSubscribeQuote Error:" << iErr << endl;
            return;
        }
    }

    // ---- ctp_api�ص����� ---- //
    // ���ӳɹ�Ӧ��
    void Quote::OnFrontConnected()
    {
        std::cout << __FUNCTION__ << std::endl;
        CThostFtdcReqUserLoginField loginReq;
        memset(&loginReq, 0, sizeof(loginReq));
        strcpy(loginReq.BrokerID, gBrokerID);
        strcpy(loginReq.UserID, gInvesterID);
        strcpy(loginReq.Password, gInvesterPassword);
        int rt = g_pMdUserApi->ReqUserLogin(&loginReq, requestID++);
        if (!rt)
            std::cout << "���͵�¼����ɹ�" << std::endl;
        else
            std::cerr << "���͵�¼����ʧ��" << std::endl;
    }

    // �Ͽ�����֪ͨ
    void Quote::OnFrontDisconnected(int nReason)
    {
        std::cerr << __FUNCTION__ << std::endl;
        std::cerr << "�����룺 " << nReason << std::endl;
    }

    // ������ʱ����
    void Quote::OnHeartBeatWarning(int nTimeLapse)
    {
        std::cerr << __FUNCTION__ << std::endl;
        std::cerr << "���ϴ�����ʱ�䣺 " << nTimeLapse << std::endl;
    }

    // ��¼Ӧ��
    void Quote::OnRspUserLogin(
        CThostFtdcRspUserLoginField *pRspUserLogin,
        CThostFtdcRspInfoField *pRspInfo,
        int nRequestID,
        bool bIsLast)
    {
        std::cout << __FUNCTION__ << std::endl;
        bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
        if (!bResult)
        {
            std::cout << "�����գ� " << pRspUserLogin->TradingDay << std::endl;
            std::cout << "��¼ʱ�䣺 " << pRspUserLogin->LoginTime << std::endl;
            std::cout << "�����̣� " << pRspUserLogin->BrokerID << std::endl;
            std::cout << "�ʻ����� " << pRspUserLogin->UserID << std::endl;
            // ��ʼ��������
            int rt = g_pMdUserApi->SubscribeMarketData(g_pInstrumentID, instrumentNum);
            if (!rt)
                std::cout << "���Ͷ�����������ɹ�" << std::endl;
            else
                std::cerr << "���Ͷ�����������ʧ��" << std::endl;
        }
        else
            std::cerr << "���ش���ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
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
        if (!bResult)
        {
            std::cout << "�˻��ǳ��ɹ�" << std::endl;
            std::cout << "�����̣� " << pUserLogout->BrokerID << std::endl;
            std::cout << "�ʻ����� " << pUserLogout->UserID << std::endl;
        }
        else
            std::cerr << "���ش���ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
    }

    // ����֪ͨ
    void Quote::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
    {
        std::cout << __FUNCTION__ << std::endl;
        bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
        if (bResult)
            std::cerr << "���ش���ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
    }

    void TAP_CDECL Quote::OnRspLogin(TAPIINT32 errorCode, const TapAPIQuotLoginRspInfo *info)
    {
        if (TAPIERROR_SUCCEED == errorCode) {
            QString log_str = "��¼�ɹ����ȴ�API��ʼ��...";
            APP_LOG(applog::LOG_INFO) << log_str.toStdString();
            emit signals_write_log(log_str);
        } else {
            QString log_str = QObject::tr("%1%2").arg("��¼ʧ�ܣ�������:").
                arg(errorCode);
            APP_LOG(applog::LOG_INFO) << log_str.toStdString();
            emit signals_write_log(log_str);
            m_Event.SignalEvent();
        }
    }

    void TAP_CDECL Quote::OnAPIReady()
    {
        QString log_str = "API��ʼ�����";
        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        emit signals_write_log(log_str);
        m_bIsAPIReady = true;
        m_Event.SignalEvent();
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

    void TAP_CDECL Quote::OnDisconnect(TAPIINT32 reasonCode)
    {
        if (!m_running) return;
        QString log_str = QObject::tr("%1%2").arg("API�Ͽ�,�Ͽ�ԭ��:").
            arg(reasonCode);
        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        emit signals_write_log(log_str);

        m_connect_state = false;
        if (m_chk_thread != nullptr) return;
        m_chk_thread = std::make_shared<std::thread>(
            std::bind(&Quote::thread_reconnect, this));
    }

    void TAP_CDECL Quote::OnRspQryCommodity(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIQuoteCommodityInfo *info)
    {
        cout << __FUNCTION__ << " is called." << endl;
        //APP_LOG(applog::LOG_INFO) << info->Commodity.ExchangeNo << " "
        //    << info->Commodity.CommodityType << " "
        //    << info->Commodity.CommodityNo;
        //if (isLast == APIYNFLAG_YES) {
        //    m_bCommodity = true;
        //    m_Event.SignalEvent();
        //}
    }

    void TAP_CDECL Quote::OnRspQryContract(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIQuoteContractInfo *info)
    {
        cout << __FUNCTION__ << " is called." << endl;
        if (info->Contract.Commodity.CommodityType == TAPI_COMMODITY_TYPE_FUTURES) {
            string key =  string(info->Contract.Commodity.CommodityNo) + 
                info->Contract.ContractNo1;
            m_map_contract[key] = info->Contract.Commodity.ExchangeNo;
        }
        //APP_LOG(applog::LOG_INFO) << info->Contract.Commodity.ExchangeNo << " "
        //    << info->Contract.Commodity.CommodityType << " "
        //    << info->Contract.Commodity.CommodityNo << " "
        //    << info->Contract.ContractNo1 << " "
        //    << info->Contract.ContractNo2;
        if (isLast == APIYNFLAG_YES) {
            m_bContract = true;
            m_Event.SignalEvent();
        }
    }


    void TAP_CDECL Quote::OnRspSubscribeQuote(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIQuoteWhole *info)
    {
        if (TAPIERROR_SUCCEED == errorCode) {
            QString log_str = "���鶩�ĳɹ�";
            APP_LOG(applog::LOG_INFO) << log_str.toStdString();
            emit signals_write_log(log_str);
            if (NULL != info) {
                emit signals_quote_changed(QString::number(info->QLastPrice, 10, 3));
           }
        } 
        else {
            QString log_str = QObject::tr("%1%2").arg("���鶩��ʧ�ܣ������룺").
                arg(errorCode);
            APP_LOG(applog::LOG_INFO) << log_str.toStdString();
            emit signals_write_log(log_str);

        }
    }

    void TAP_CDECL Quote::OnRspUnSubscribeQuote(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIContract *info)
    {
        cout << __FUNCTION__ << " is called." << endl;
    }

    void TAP_CDECL Quote::OnRtnQuote(const TapAPIQuoteWhole *info)
    {
        if (NULL != info) {
            emit signals_quote_changed(QString::number(info->QLastPrice, 10, 3));
        }
    }
}