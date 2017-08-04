/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: Trade.cpp
Version: 1.0
Date: 2017.4.25

History:
shengkaishan     2017.4.25   1.0     Create
******************************************************************************/


#include "Trade.h"
#include "TradeConfig.h"
#include <iostream>
#include <string.h>
#include "common.h"
#include "applog.h"
#include "trader_server.h"
#include "future_platform.h"

using namespace std;
namespace future
{
    Trade::Trade(void) :
        m_pAPI(NULL),
        m_requestid(0),
        m_brokerid(""),
        m_investorid(""),
        m_bfront_status(false),
        m_blogin_status(false),
        m_bconfirm_status(false),
        m_bposition(false),
        m_border(false),
        m_connect_state(false),
        m_chk_thread(nullptr),
        m_running(true)
    {
        m_map_order.clear();
    }

    Trade::~Trade(void)
    {
        m_map_order.clear();
    }

    void Trade::SetAPI(CThostFtdcTraderApi *pAPI)
    {
        m_pAPI = pAPI;
    }

    void Trade::Run()
    {
        if (NULL == m_pAPI) {
            cout << "Error: m_pAPI is NULL." << endl;
            return;
        }

        int ret = TAPIERROR_SUCCEED;
        //���ӷ�����IP���˿�
        QString log_str = "�������ӽ��׷���";
        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        emit signals_write_log(log_str);
        string key = "trader_info/ip";
        QString ip = common::get_config_value(key).toString();
        key = "trader_info/port";
        int port = common::get_config_value(key).toInt();
        string addr;
        addr.append("tcp://").append(ip.toStdString()).append(":").append(std::to_string(port));
        m_pAPI->RegisterSpi(this);                        // ע���¼���
        m_pAPI->RegisterFront(const_cast<char*>(addr.c_str()));                // ���ý���ǰ�õ�ַ
        m_pAPI->SubscribePublicTopic(THOST_TERT_RESTART);      // ���Ĺ�����
        m_pAPI->SubscribePrivateTopic(THOST_TERT_RESUME);     // ����˽����
        m_pAPI->Init();                                        // ��������
        //�ȴ�m_bfront_status
        m_Event.WaitEvent();
        if (!m_bfront_status) {
            return;
        }

        //��¼������
        log_str = "���ڵ�¼���׷���";
        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        emit signals_write_log(log_str);
        key = "trader_info/brokerid";
        m_brokerid = common::get_config_value(key).toString().toStdString();
        key = "trader_info/userid";
        m_investorid = common::get_config_value(key).toString().toStdString();
        key = "trader_info/passwd";
        //QString passwd = common::getXorEncryptDecrypt(
        //    common::get_config_value(key).toString());
        QString passwd = common::get_config_value(key).toString();
        CThostFtdcReqUserLoginField loginReq;
        memset(&loginReq, 0, sizeof(loginReq));
        strcpy(loginReq.BrokerID, m_brokerid.c_str());
        strcpy(loginReq.UserID, m_investorid.c_str());
        strcpy(loginReq.Password, passwd.toStdString().c_str());
        ret = m_pAPI->ReqUserLogin(&loginReq, m_requestid++);
        if (TAPIERROR_SUCCEED != ret) {
            cout << "ReqUserLogin Error:" << ret << endl;
            return;
        }
        //�ȴ�m_blogin_status
        m_Event.WaitEvent();
        if (!m_blogin_status) {
            return;
        }

        //Ͷ���߽�����ȷ��
        log_str = "���ڽ�����ȷ��";
        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        emit signals_write_log(log_str);
        CThostFtdcSettlementInfoConfirmField settlementConfirmReq;
        memset(&settlementConfirmReq, 0, sizeof(settlementConfirmReq));
        strcpy(settlementConfirmReq.BrokerID, m_brokerid.c_str());
        strcpy(settlementConfirmReq.InvestorID, m_investorid.c_str());
        ret = m_pAPI->ReqSettlementInfoConfirm(&settlementConfirmReq, m_requestid++);
        if (TAPIERROR_SUCCEED != ret) {
            cout << "ReqSettlementInfoConfirm Error:" << ret << endl;
            return;
        }
        //�ȴ�m_bconfirm_status
        m_Event.WaitEvent();
        if (!m_bconfirm_status) {
            return;
        }

        ret = qry_postion();
        if (TAPIERROR_SUCCEED != ret) {
            cout << "qry_postion Error:" << ret << endl;
            return;
        }
        //�ȴ�qry_postion
        m_Event.WaitEvent();
        if (!m_bposition) {
            return;
        }

        ret = qry_order();
        if (TAPIERROR_SUCCEED != ret) {
            cout << "qry_order Error:" << ret << endl;
            return;
        }
        m_Event.WaitEvent();
        if (!m_border) {
            return;
        }

        log_str = "���׷����¼���";
        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        emit signals_write_log(log_str);

        m_connect_state = true;
    }

    int Trade::qry_postion()
    {
        CThostFtdcQryInvestorPositionField postionReq;
        memset(&postionReq, 0, sizeof(postionReq));
        strcpy(postionReq.BrokerID, m_brokerid.c_str());
        strcpy(postionReq.InvestorID, m_investorid.c_str());
        //strcpy(postionReq.InstrumentID, g_pTradeInstrumentID);
        std::this_thread::sleep_for(std::chrono::milliseconds(700)); // ��ʱ����Ҫͣ��һ����ܲ�ѯ�ɹ�
        return m_pAPI->ReqQryInvestorPosition(&postionReq, m_requestid++);
    }

    int Trade::qry_order()
    {
        CThostFtdcQryOrderField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.BrokerID, m_brokerid.c_str());
        strcpy(req.InvestorID, m_investorid.c_str());

        return m_pAPI->ReqQryOrder(&req, m_requestid++);
    }

    void Trade::order_open(string& account, string& contract, double price)
    {
        QString log_str = QObject::tr("%1%2 %3").arg("�ҵ���").arg(contract.c_str()).arg(price);
        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        emit signals_write_log(log_str);

        int ret = TAPIERROR_SUCCEED;
        //�µ�
        CThostFtdcInputOrderField orderInsertReq;
        memset(&orderInsertReq, 0, sizeof(orderInsertReq));
        ///���͹�˾����
        strcpy(orderInsertReq.BrokerID, m_brokerid.c_str());
        ///Ͷ���ߴ���
        strcpy(orderInsertReq.InvestorID, m_investorid.c_str());
        ///��Լ����
        strcpy(orderInsertReq.InstrumentID, contract.c_str());
        ///��������
        strcpy(orderInsertReq.OrderRef, std::to_string(m_requestid).c_str());
        ///�����۸�����: �޼�
        orderInsertReq.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
        ///��������: 
        orderInsertReq.Direction = THOST_FTDC_D_Sell;
        ///��Ͽ�ƽ��־: ����
        orderInsertReq.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
        ///���Ͷ���ױ���־
        orderInsertReq.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
        ///�۸�
        orderInsertReq.LimitPrice = price;
        ///������1
        orderInsertReq.VolumeTotalOriginal = 1;
        ///��Ч������: ������Ч
        orderInsertReq.TimeCondition = THOST_FTDC_TC_GFD;
        ///�ɽ�������: �κ�����
        orderInsertReq.VolumeCondition = THOST_FTDC_VC_AV;
        ///��С�ɽ���: 1
        orderInsertReq.MinVolume = 1;
        ///��������: ����
        orderInsertReq.ContingentCondition = THOST_FTDC_CC_Immediately;
        ///ǿƽԭ��: ��ǿƽ
        orderInsertReq.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
        ///�Զ������־: ��
        orderInsertReq.IsAutoSuspend = 0;
        ///�û�ǿ����־: ��
        orderInsertReq.UserForceClose = 0;

        ret = m_pAPI->ReqOrderInsert(&orderInsertReq, m_requestid++);
        if (TAPIERROR_SUCCEED != ret) {
            QString log_str = QObject::tr("%1%2").arg("����ʧ�ܣ�������:").
                arg(ret);
            APP_LOG(applog::LOG_INFO) << log_str.toStdString();
            emit signals_write_log(log_str);
            return;
        }
    }
    void Trade::order_withdraw()
    {
        int ret = TAPIERROR_SUCCEED;
        string key = "order_info/ExchangeID";
        QString ExchangeID = common::get_config_value(key).toString();
        key = "order_info/OrderSysID";
        QString OrderSysID = common::get_config_value(key).toString();

        QString log_str = QObject::tr("%1%2").arg("��������ˮ��:").arg(OrderSysID);
        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        emit signals_write_log(log_str);

        CThostFtdcInputOrderActionField orderActionReq;
        memset(&orderActionReq, 0, sizeof(orderActionReq));
        ///���͹�˾����
        strcpy(orderActionReq.BrokerID, m_brokerid.c_str());
        ///Ͷ���ߴ���
        strcpy(orderActionReq.InvestorID, m_investorid.c_str());
        ///����������
        strcpy(orderActionReq.ExchangeID, ExchangeID.toStdString().c_str());
        ///�������
        strcpy(orderActionReq.OrderSysID, OrderSysID.toStdString().c_str());
        ///������־
        orderActionReq.ActionFlag = THOST_FTDC_AF_Delete;

        ret = m_pAPI->ReqOrderAction(&orderActionReq, m_requestid++);
        if (TAPIERROR_SUCCEED != ret) {
            QString log_str = QObject::tr("%1%2").arg("����ʧ�ܣ�������:").
                arg(ret);
            APP_LOG(applog::LOG_INFO) << log_str.toStdString();
            emit signals_write_log(log_str);
            return;
        }
    }
    void Trade::order_close(string& account, string& contract, double price)
    {
        QString log_str = QObject::tr("%1%2").arg("�м�ƽ��").arg(contract.c_str());
        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        emit signals_write_log(log_str);
        int ret = TAPIERROR_SUCCEED;

        CThostFtdcInputOrderField orderInsertReq;
        memset(&orderInsertReq, 0, sizeof(orderInsertReq));
        ///���͹�˾����
        strcpy(orderInsertReq.BrokerID, m_brokerid.c_str());
        ///Ͷ���ߴ���
        strcpy(orderInsertReq.InvestorID, m_investorid.c_str());
        ///��Լ����
        strcpy(orderInsertReq.InstrumentID, contract.c_str());
        ///��������
        strcpy(orderInsertReq.OrderRef, std::to_string(m_requestid).c_str());
        ///�����۸�����: �޼�
        orderInsertReq.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
        ///��������: 
        orderInsertReq.Direction = THOST_FTDC_D_Buy;
        ///��Ͽ�ƽ��־: ����
        orderInsertReq.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
        ///���Ͷ���ױ���־
        orderInsertReq.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
        ///�۸�
        orderInsertReq.LimitPrice = price;
        ///������1
        orderInsertReq.VolumeTotalOriginal = DEFAULT_ORDER_QTY;
        ///��Ч������: ������Ч
        orderInsertReq.TimeCondition = THOST_FTDC_TC_IOC;
        ///�ɽ�������: �κ�����
        orderInsertReq.VolumeCondition = THOST_FTDC_VC_AV;
        ///��С�ɽ���: 1
        orderInsertReq.MinVolume = 1;
        ///��������: ����
        orderInsertReq.ContingentCondition = THOST_FTDC_CC_Immediately;
        ///ǿƽԭ��: ��ǿƽ
        orderInsertReq.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
        ///�Զ������־: ��
        orderInsertReq.IsAutoSuspend = 0;
        ///�û�ǿ����־: ��
        orderInsertReq.UserForceClose = 0;

        ret = m_pAPI->ReqOrderInsert(&orderInsertReq, m_requestid++);
        if (TAPIERROR_SUCCEED != ret) {
            QString log_str = QObject::tr("%1%2").arg("��ƽʧ�ܣ�������:").
                arg(ret);
            APP_LOG(applog::LOG_INFO) << log_str.toStdString();
            emit signals_write_log(log_str);
            return;
        }
    }

    void Trade::order_state_handle(const CThostFtdcOrderField *info)
    {
        //! ί��״̬����
        //#define THOST_FTDC_OST_AllTraded '0'                    ///ȫ���ɽ� (����״̬)
        //#define THOST_FTDC_OST_PartTradedQueueing '1'           ///���ֳɽ����ڶ����� ���ɳ�����
        //#define THOST_FTDC_OST_PartTradedNotQueueing '2'        ///���ֳɽ����ڶ����� (����״̬)
        //#define THOST_FTDC_OST_NoTradeQueueing '3'              ///δ�ɽ����ڶ����� ���ɳ�����
        //#define THOST_FTDC_OST_NoTradeNotQueueing '4'           ///δ�ɽ����ڶ����� (����״̬)
        //#define THOST_FTDC_OST_Canceled '5'                     ///���� (����״̬)
        //#define THOST_FTDC_OST_Unknown 'a'                      ///δ֪ (�ɽ�ʱ������)
        //#define THOST_FTDC_OST_NotTouched 'b'                   ///��δ���� ��
        //#define THOST_FTDC_OST_Touched 'c'                      ///�Ѵ��� ��
        switch (info->OrderStatus) {
        case THOST_FTDC_OST_Unknown:
        case THOST_FTDC_OST_NotTouched:
        {
            QString log_str = QObject::tr("%1").arg("ί��������");
            APP_LOG(applog::LOG_INFO) << log_str.toStdString();
            emit signals_write_log(log_str);
            break;
        }
        case THOST_FTDC_OST_NoTradeQueueing:
        case THOST_FTDC_OST_NoTradeNotQueueing: //����״̬��
        case THOST_FTDC_OST_Touched:
        {
            string key = "order_info/ExchangeID";
            common::set_config_value(key, string(info->ExchangeID));
            key = "order_info/OrderSysID";
            common::set_config_value(key, string(info->OrderSysID));

            emit signals_state_changed(THOST_FTDC_OST_Touched,
                '0',
                QString::number(0, 10, 0));

            QString log_str = QObject::tr("%1%2").arg("ί�����Ŷ�,��ˮ��:").
                arg(info->OrderSysID);
            APP_LOG(applog::LOG_INFO) << log_str.toStdString();
            emit signals_write_log(log_str);
            break;
        }
        case THOST_FTDC_OST_PartTradedQueueing:
        case THOST_FTDC_OST_PartTradedNotQueueing: //����״̬��
        {
            APP_LOG(applog::LOG_INFO) << "���ֳɽ�";
            break;
        }
        case THOST_FTDC_OST_AllTraded:
        {
            APP_LOG(applog::LOG_INFO) << "��ȫ�ɽ�";
            //emit signals_state_changed(THOST_FTDC_OST_AllTraded,
            //    '0',
            //    QString::number(0, 10, 0));
            break;
        }
        case THOST_FTDC_OST_Canceled:
        {
            emit signals_state_changed(THOST_FTDC_OST_Canceled,
                '0',
                QString::number(0, 10, 0));

            QString log_str = QObject::tr("%1%2%3%4").arg("�����ɹ�,��ˮ��:").
                arg(info->OrderSysID).arg("״̬��Ϣ��").arg(QString::fromLocal8Bit(info->StatusMsg));;
            APP_LOG(applog::LOG_INFO) << log_str.toStdString();
            emit signals_write_log(log_str);
            break;
        }
        default:
            break;
        }
    }
    void Trade::thread_reconnect()
    {
        while (m_running) {
            if (!m_connect_state) {
                Run();
            }
            Sleep(3000);
        }
    }

    bool Trade::is_error_rsp(CThostFtdcRspInfoField *pRspInfo)
    {
        bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
        if (bResult)
            std::cerr << "���ش���ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
        return bResult;
    }

    void Trade::OnFrontConnected()
    {
        QString log_str = "����API���ӳɹ�";
        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        emit signals_write_log(log_str);
        m_bfront_status = true;
        m_Event.SignalEvent();
    }

    void Trade::OnRspUserLogin(
        CThostFtdcRspUserLoginField *pRspUserLogin,
        CThostFtdcRspInfoField *pRspInfo,
        int nRequestID,
        bool bIsLast)
    {
        if (!is_error_rsp(pRspInfo)) {
            QString log_str = "����API��¼�ɹ�";
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

    void Trade::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
    {
        is_error_rsp(pRspInfo);
    }

    void Trade::OnFrontDisconnected(int nReason)
    {
        if (!m_running) return;
        QString log_str = QObject::tr("%1%2").arg("����API�Ͽ�,�Ͽ�ԭ��:").
            arg(nReason);
        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        emit signals_write_log(log_str);

        m_connect_state = false;
        if (m_chk_thread != nullptr) return;
        m_chk_thread = std::make_shared<std::thread>(
            std::bind(&Trade::thread_reconnect, this));
    }

    void Trade::OnHeartBeatWarning(int nTimeLapse)
    {
        std::cerr << "=====����������ʱ=====" << std::endl;
        std::cerr << "���ϴ�����ʱ�䣺 " << nTimeLapse << std::endl;
    }

    void Trade::OnRspUserLogout(
        CThostFtdcUserLogoutField *pUserLogout,
        CThostFtdcRspInfoField *pRspInfo,
        int nRequestID,
        bool bIsLast)
    {
        std::cout << __FUNCTION__ << std::endl;
        if (!is_error_rsp(pRspInfo)) {
            std::cout << "�˻��ǳ��ɹ�" << std::endl;
            std::cout << "�����̣� " << pUserLogout->BrokerID << std::endl;
            std::cout << "�ʻ����� " << pUserLogout->UserID << std::endl;
        }
        else {
            std::cerr << "���ش���ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
        }
    }

    void Trade::OnRspSettlementInfoConfirm(
        CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
        CThostFtdcRspInfoField *pRspInfo,
        int nRequestID,
        bool bIsLast)
    {
        if (!is_error_rsp(pRspInfo)) {
            QString log_str = "����API������ȷ�ϳɹ�";
            APP_LOG(applog::LOG_INFO) << log_str.toStdString();
            emit signals_write_log(log_str);
            m_bconfirm_status = true;
        }
        else {
            QString log_str = QObject::tr("%1%2").arg("������ȷ��ʧ�ܣ�������:").
                arg(pRspInfo->ErrorID);
            APP_LOG(applog::LOG_INFO) << log_str.toStdString();
            emit signals_write_log(log_str);

        }
        m_Event.SignalEvent();

    }

    void Trade::OnRspQryInstrument(
        CThostFtdcInstrumentField *pInstrument,
        CThostFtdcRspInfoField *pRspInfo,
        int nRequestID,
        bool bIsLast)
    {
        if (!is_error_rsp(pRspInfo)) {
            std::cout << "=====��ѯ��Լ����ɹ�=====" << std::endl;
            std::cout << "���������룺 " << pInstrument->ExchangeID << std::endl;
            std::cout << "��Լ���룺 " << pInstrument->InstrumentID << std::endl;
            std::cout << "��Լ�ڽ������Ĵ��룺 " << pInstrument->ExchangeInstID << std::endl;
            std::cout << "ִ�мۣ� " << pInstrument->StrikePrice << std::endl;
            std::cout << "�����գ� " << pInstrument->EndDelivDate << std::endl;
            std::cout << "��ǰ����״̬�� " << pInstrument->IsTrading << std::endl;
        }
    }

    void Trade::OnRspQryInvestorPosition(
        CThostFtdcInvestorPositionField *pInvestorPosition,
        CThostFtdcRspInfoField *pRspInfo,
        int nRequestID,
        bool bIsLast)
    {
        if (!is_error_rsp(pRspInfo)) {
            if (pInvestorPosition == nullptr) {
                m_bposition = true;
                m_Event.SignalEvent();
                return;
            }

            if (!bIsLast) {
                CThostFtdcInputOrderField orderInsertReq;
                memset(&orderInsertReq, 0, sizeof(orderInsertReq));
                ///���͹�˾����
                strcpy(orderInsertReq.BrokerID, m_brokerid.c_str());
                ///Ͷ���ߴ���
                strcpy(orderInsertReq.InvestorID, m_investorid.c_str());
                ///��Լ����
                strcpy(orderInsertReq.InstrumentID, pInvestorPosition->InstrumentID);
                ///��������
                strcpy(orderInsertReq.OrderRef, std::to_string(m_requestid).c_str());
                ///�����۸�����: �޼�
                orderInsertReq.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
                ///��������: 
                orderInsertReq.Direction = (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Short) ?
                THOST_FTDC_D_Buy : THOST_FTDC_D_Sell;
                ///��Ͽ�ƽ��־: ����
                orderInsertReq.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
                ///���Ͷ���ױ���־
                orderInsertReq.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
                ///�۸�
                orderInsertReq.LimitPrice = pInvestorPosition->PreSettlementPrice + 100;
                ///������1
                orderInsertReq.VolumeTotalOriginal = pInvestorPosition->Position;
                ///��Ч������: ������Ч
                orderInsertReq.TimeCondition = THOST_FTDC_TC_GFD;
                ///�ɽ�������: �κ�����
                orderInsertReq.VolumeCondition = THOST_FTDC_VC_AV;
                ///��С�ɽ���: 1
                orderInsertReq.MinVolume = 1;
                ///��������: ����
                orderInsertReq.ContingentCondition = THOST_FTDC_CC_Immediately;
                ///ǿƽԭ��: ��ǿƽ
                orderInsertReq.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
                ///�Զ������־: ��
                orderInsertReq.IsAutoSuspend = 0;
                ///�û�ǿ����־: ��
                orderInsertReq.UserForceClose = 0;

                int ret = m_pAPI->ReqOrderInsert(&orderInsertReq, m_requestid++);
                if (TAPIERROR_SUCCEED != ret) {
                    QString log_str = QObject::tr("%1%2").arg("��ƽʧ�ܣ�������:").
                        arg(ret);
                    APP_LOG(applog::LOG_INFO) << log_str.toStdString();
                    emit signals_write_log(log_str);
                    return;
                }
            }
            else {
                emit signals_close_position("", pInvestorPosition->InstrumentID);
                m_bposition = true;
                m_Event.SignalEvent();
            }
        }
    }

    void Trade::OnRspQryOrder(CThostFtdcOrderField *pOrder, 
        CThostFtdcRspInfoField *pRspInfo, 
        int nRequestID, 
        bool bIsLast)
    {
        if (!is_error_rsp(pRspInfo)) {
            if (pOrder == nullptr) {
                m_border = true;
                m_Event.SignalEvent();
                return;
            }

            if (pOrder->OrderStatus == THOST_FTDC_OST_PartTradedQueueing ||
                pOrder->OrderStatus == THOST_FTDC_OST_NoTradeQueueing) {
                m_map_order[pOrder->OrderSysID] = pOrder->ExchangeID;
            }

            if (bIsLast) {
                if (m_map_order.size() != 0) {
                    auto last = m_map_order.end();
                    last--;
                    for (auto it = m_map_order.begin(); it != last; it++) {
                        CThostFtdcInputOrderActionField orderActionReq;
                        memset(&orderActionReq, 0, sizeof(orderActionReq));
                        ///���͹�˾����
                        strcpy(orderActionReq.BrokerID, m_brokerid.c_str());
                        ///Ͷ���ߴ���
                        strcpy(orderActionReq.InvestorID, m_investorid.c_str());
                        ///����������
                        strcpy(orderActionReq.ExchangeID, it->second.c_str());
                        ///�������
                        strcpy(orderActionReq.OrderSysID, it->first.c_str());
                        ///������־
                        orderActionReq.ActionFlag = THOST_FTDC_AF_Delete;

                        int ret = m_pAPI->ReqOrderAction(&orderActionReq, m_requestid++);
                        if (TAPIERROR_SUCCEED != ret) {
                            QString log_str = QObject::tr("%1%2").arg("����ʧ�ܣ�������:").
                                arg(ret);
                            APP_LOG(applog::LOG_INFO) << log_str.toStdString();
                            emit signals_write_log(log_str);
                            return;
                        }
                    }
                    string key = "order_info/ExchangeID";
                    common::set_config_value(key, last->second);
                    key = "order_info/OrderSysID";
                    common::set_config_value(key, string(last->first));
                    emit signals_withdraw_order(last->first.c_str());
                }


                m_border = true;
                m_Event.SignalEvent();
            }
        }
    }

    void Trade::OnRspQryTrade(CThostFtdcTradeField *pTrade, 
        CThostFtdcRspInfoField *pRspInfo, 
        int nRequestID, 
        bool bIsLast)
    {
        std::cout << __FUNCTION__ << std::endl;
    }

    void Trade::OnRspOrderInsert(
        CThostFtdcInputOrderField *pInputOrder,
        CThostFtdcRspInfoField *pRspInfo,
        int nRequestID,
        bool bIsLast)
    {
        std::cout << __FUNCTION__ << std::endl;
        if (!is_error_rsp(pRspInfo)) {
            std::cout << "=====����¼��ɹ�=====" << std::endl;
            std::cout << "��Լ���룺 " << pInputOrder->InstrumentID << std::endl;
            std::cout << "�۸� " << pInputOrder->LimitPrice << std::endl;
            std::cout << "������ " << pInputOrder->VolumeTotalOriginal << std::endl;
            std::cout << "���ַ��� " << pInputOrder->Direction << std::endl;
        } 
        else {
            QString log_str = QObject::tr("���׺��ķ��ر���ʧ��,������:%1,������Ϣ:%2").
                arg(pRspInfo->ErrorID).
                arg(QString::fromLocal8Bit(pRspInfo->ErrorMsg));
            APP_LOG(applog::LOG_INFO) << log_str.toStdString();
            emit signals_write_log(log_str);
        }
    }

    void Trade::OnRspOrderAction(
        CThostFtdcInputOrderActionField *pInputOrderAction,
        CThostFtdcRspInfoField *pRspInfo,
        int nRequestID,
        bool bIsLast)
    {
        std::cout << __FUNCTION__ << std::endl;
        if (!is_error_rsp(pRspInfo)) {
            std::cout << "=====���������ɹ�=====" << std::endl;
            std::cout << "��Լ���룺 " << pInputOrderAction->InstrumentID << std::endl;
            std::cout << "������־�� " << pInputOrderAction->ActionFlag;
        } 
        else {
            QString log_str = QObject::tr("���׺��ķ��س���ʧ��,������:%1,������Ϣ:%2").
                arg(pRspInfo->ErrorID).
                arg(QString::fromLocal8Bit(pRspInfo->ErrorMsg));
            APP_LOG(applog::LOG_INFO) << log_str.toStdString();
            emit signals_write_log(log_str);
        }
    }

    void Trade::OnRtnOrder(CThostFtdcOrderField *pOrder)
    {
        std::cout << __FUNCTION__ << std::endl;
        std::cout << "=====�յ�����Ӧ��=====" << std::endl;
        APP_LOG(applog::LOG_INFO)
            << "OrderRef�� " << pOrder->OrderRef << " "
            << "FrontID�� " << pOrder->FrontID << " "
            << "SessionID�� " << pOrder->SessionID << " "
            << "Direction�� " << pOrder->Direction << " "
            << "CombOffsetFlag�� " << pOrder->CombOffsetFlag << " "
            << "CombHedgeFlag�� " << pOrder->CombHedgeFlag << " "
            << "LimitPrice�� " << pOrder->LimitPrice << " "
            << "VolumeTotalOriginal�� " << pOrder->VolumeTotalOriginal << " "
            << "RequestID�� " << pOrder->RequestID << " "
            << "OrderLocalID�� " << pOrder->OrderLocalID << " "
            << "ExchangeID�� " << pOrder->ExchangeID << " "
            << "TraderID�� " << pOrder->TraderID << " "
            << "OrderSubmitStatus�� " << pOrder->OrderSubmitStatus << " "
            << "TradingDay�� " << pOrder->TradingDay << " "
            << "OrderSysID�� " << pOrder->OrderSysID << " "
            << "OrderStatus�� " << pOrder->OrderStatus << " "
            << "OrderType�� " << pOrder->OrderType << " "
            << "VolumeTraded�� " << pOrder->VolumeTraded << " "
            << "VolumeTotal�� " << pOrder->VolumeTotal << " "
            << "InsertDate�� " << pOrder->InsertDate << " "
            << "InsertTime�� " << pOrder->InsertTime << " "
            << "SequenceNo�� " << pOrder->SequenceNo << " "
            << "StatusMsg�� " << QString::fromLocal8Bit(pOrder->StatusMsg).toStdString();
        if (NULL == pOrder) return;
        //if (pOrder->OrderStatus != THOST_FTDC_OST_Canceled
        //    && pOrder->OrderStatus != THOST_FTDC_OST_AllTraded
        //    && pOrder->OrderStatus != THOST_FTDC_OST_PartTradedQueueing
        //    && pOrder->OrderStatus != THOST_FTDC_OST_PartTradedNotQueueing) {
        //        APP_LOG(applog::LOG_INFO) << "�����ɹ���"
        //            << "״̬:" << pOrder->OrderStatus << ","
        //            << "ί�б��:" << pOrder->OrderSysID ;
        //        QString log_str = QObject::tr("�����ɹ�,״̬:%1,ί�б��:%2").
        //            arg(pOrder->OrderStatus).
        //            arg(pOrder->OrderSysID);
        //        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        //        emit signals_write_log(log_str);
        //}
        order_state_handle(pOrder);
    }

    void Trade::OnRtnTrade(CThostFtdcTradeField *pTrade)
    {
        std::cout << __FUNCTION__ << std::endl;
        std::cout << "=====�����ɹ��ɽ�=====" << std::endl;
        APP_LOG(applog::LOG_INFO)
            << "OrderRef�� " << pTrade->OrderRef << " "
            << "ExchangeID�� " << pTrade->ExchangeID << " "
            << "TradeID�� " << pTrade->TradeID << " "
            << "Direction�� " << pTrade->Direction << " "
            << "OrderSysID�� " << pTrade->OrderSysID << " "
            << "OffsetFlag�� " << pTrade->OffsetFlag << " "
            << "HedgeFlag�� " << pTrade->HedgeFlag << " "
            << "Price�� " << pTrade->Price << " "
            << "Volume�� " << pTrade->Volume << " "
            << "TradeDate�� " << pTrade->TradeDate << " "
            << "TradeTime�� " << pTrade->TradeTime << " "
            << "TradeType�� " << pTrade->TradeType << " "
            << "TraderID�� " << pTrade->TraderID << " "
            << "OrderLocalID�� " << pTrade->OrderLocalID << " "
            << "SequenceNo�� " << pTrade->SequenceNo << " "
            << "TradingDay�� " << pTrade->TradingDay << " "
            << "TradeSource�� " << pTrade->TradeSource;

        emit signals_state_changed(THOST_FTDC_OST_AllTraded,
            pTrade->Direction,
            QString::number(pTrade->Price, 10, 2));

        QString log_str;
        if (pTrade->Direction == THOST_FTDC_D_Sell) {
            log_str = QObject::tr("��%1���׳ɹ�����ˮ��%2").
                arg(pTrade->Price).
                arg(pTrade->TradeID);
        }
        else {
            log_str = QObject::tr("%1ƽ�ֳɹ�����ˮ��%2").
                arg(pTrade->Price).
                arg(pTrade->TradeID);
        }
        APP_LOG(applog::LOG_INFO) << log_str.toStdString();
        emit signals_write_log(log_str);
    }


    ///����¼�����ر�
    void Trade::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
    {
        std::cout << __FUNCTION__ << std::endl;
    }

    ///������������ر�
    void Trade::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo)
    {
        std::cout << __FUNCTION__ << std::endl;
    }
}