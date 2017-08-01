/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: Quote.h
Version: 1.0
Date: 2017.4.25

History:
shengkaishan     2017.4.25   1.0     Create
******************************************************************************/


#ifndef QUOTE_H
#define QUOTE_H

#include "ThostFtdcMdApi.h"
#include "SimpleEvent.h"
#include <map>
#include <QObject>
#include <QString>
#include <thread>
#include <memory>
#include <atomic>
using namespace std;
namespace future
{
    class Quote : public QObject, public CThostFtdcMdSpi
    {
        Q_OBJECT
    public:
        Quote(void);
        ~Quote(void);

        void SetAPI(CThostFtdcMdApi* pAPI);
        int Run();

        void req_sub_market_data(string& contract);
        void req_unsub_market_data(string& contract);

    signals:
        void signals_write_log(QString str);
        void signals_quote_changed(QString last_price);
        void signals_quote_reconnect();

    private:
        void thread_reconnect();

    public:
        ///���ͻ����뽻�׺�̨������ͨ������ʱ����δ��¼ǰ�����÷��������á�
        void OnFrontConnected();

        ///���ͻ����뽻�׺�̨ͨ�����ӶϿ�ʱ���÷��������á���������������API���Զ��������ӣ��ͻ��˿ɲ�������
        ///@param nReason ����ԭ��
        ///        0x1001 �����ʧ��
        ///        0x1002 ����дʧ��
        ///        0x2001 ����������ʱ
        ///        0x2002 ��������ʧ��
        ///        0x2003 �յ�������
        void OnFrontDisconnected(int nReason);

        ///������ʱ���档����ʱ��δ�յ�����ʱ���÷��������á�
        ///@param nTimeLapse �����ϴν��ձ��ĵ�ʱ��
        void OnHeartBeatWarning(int nTimeLapse);

        ///��¼������Ӧ
        void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        ///�ǳ�������Ӧ
        void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        ///����Ӧ��
        void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        ///��������Ӧ��
        void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        ///ȡ����������Ӧ��
        void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        ///�������֪ͨ
        void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);

    private:
        CThostFtdcMdApi* m_pAPI;
        int    request_id;
        SimpleEvent m_Event;
        bool        m_bfront_status;
        bool        m_bIsAPIReady;

        map<string, string> m_map_contract;
        bool        m_connect_state;
    public:
        atomic<bool> m_running;
        std::shared_ptr<std::thread> m_chk_thread;
    };
}
#endif // QUOTE_H
