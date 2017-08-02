/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: Trade.h
Version: 1.0
Date: 2017.4.25

History:
shengkaishan     2017.4.25   1.0     Create
******************************************************************************/


#ifndef TRADE_H
#define TRADE_H

#include "ThostFtdcTraderApi.h"
#include "SimpleEvent.h"
#include <map>
#include<QObject>
#include<QString>
#include <memory>
#include <thread>
#include <atomic>
using namespace std;
namespace future
{
    class Trade : public QObject, public CThostFtdcTraderSpi
    {
        Q_OBJECT
    public:
        Trade(void);
        ~Trade(void);

        void SetAPI(CThostFtdcTraderApi *pAPI);
        void Run();

        //void qry_postion();
        //void qry_order();
        void order_open(string& account, string& contract, double price);
        void order_withdraw();
        void order_close(string& account, string& contract);

    signals:
        void signals_write_log(QString str);
        void signals_state_changed(char state, char effect, QString deal_price);
        void signals_close_position(QString commodity_no, QString contract_no);
        void signals_withdraw_order(QString order_no);

    private:
        void order_state_handle(const CThostFtdcOrderField *info);
        void thread_reconnect();

        bool is_error_rsp(CThostFtdcRspInfoField *pRspInfo); // �Ƿ��յ�������Ϣ
    public:
        ///���ͻ����뽻�׺�̨������ͨ������ʱ����δ��¼ǰ�����÷��������á�
        void OnFrontConnected();

        ///��¼������Ӧ
        void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        ///����Ӧ��
        void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        ///���ͻ����뽻�׺�̨ͨ�����ӶϿ�ʱ���÷��������á���������������API���Զ��������ӣ��ͻ��˿ɲ�������
        void OnFrontDisconnected(int nReason);

        ///������ʱ���档����ʱ��δ�յ�����ʱ���÷��������á�
        void OnHeartBeatWarning(int nTimeLapse);

        ///�ǳ�������Ӧ
        void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        ///Ͷ���߽�����ȷ����Ӧ
        void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        ///�����ѯ��Լ��Ӧ
        void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        ///�����ѯͶ���ֲ߳���Ӧ
        void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        ///����¼��������Ӧ
        void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        ///��������������Ӧ
        void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        ///����֪ͨ
        void OnRtnOrder(CThostFtdcOrderField *pOrder);

        ///�ɽ�֪ͨ
        void OnRtnTrade(CThostFtdcTradeField *pTrade);

        ///����¼�����ر�
        virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo);

        ///������������ر�
        virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo);

    private:
        CThostFtdcTraderApi *m_pAPI;
        int    m_requestid;
        string m_brokerid;
        string m_investorid;
        SimpleEvent m_Event;
        bool        m_bfront_status;
        bool        m_blogin_status;
        bool        m_bconfirm_status;
        bool        m_bposition;
        bool        m_border;
        bool        m_connect_state;
        map<string, char> m_map_order;
    public:
        atomic<bool> m_running;
        std::shared_ptr<std::thread> m_chk_thread;
    };
}
#endif // TRADE_H
