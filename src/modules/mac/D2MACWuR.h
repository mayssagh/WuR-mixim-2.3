//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef __MIXIM_D2MACWuR_H_
#define __MIXIM_D2MACWuR_H_

#include <omnetpp.h>
#include <string>
#include <sstream>
#include <vector>
#include <list>

#include "MiXiMDefs.h"
#include "BaseMacLayer.h"
#include "DroppedPacket.h"
#include "Packet.h"
#include "SimpleAddress.h"

class MacPkt;

/**
 * TODO - Generated class
 */
class MIXIM_API D2MACWuR : public BaseMacLayer
{
public:
    D2MACWuR()
        : BaseMacLayer()
        , nbTxWuC(0)
        , nbTxWuCCSMA(0)
        , nbRxWuC(0)
        , nbRxWuCCSMA(0)
        , nbMissedAcks(0)
        , nbRecvdAcks(0)
        , nbDroppedWuC(0)
        , nbDroppedWuCCSMA(0)
        , nbTxAcks(0)
        , nbDuplicates(0)
        , nbBackoffs(0)
        , backoffValues(0)
        , stats(false)
        , trace(false)
        , backoffTimer(NULL), ccaTimer(NULL), sifsTimer(NULL), rxAckTimer(NULL)
        , backoffTimerPCA(NULL), ccaTimerPCA(NULL), sifsTimerPCA(NULL), rxAckTimerPCA(NULL)
        , macState(IDLE_1)
        , macStatePCA(IDLE_1_PCA)
        , status(STATUS_OK)
        , sifs()
        , macAckWaitDuration()
        , transmissionAttemptInterruptedByRx(false)
        , ccaDetectionTime()
        , rxSetupTime()
        , aTurnaroundTime()
        , macMaxCSMABackoffs(0)
        , macMaxFrameRetries(0)
        , aUnitBackoffPeriod()
        , useMACAcks(false)
        , backoffMethod(CONSTANT)
        , macMinBE(0)
        , macMaxBE(0)
        , initialCW(0)
        , txPower(0)
        , NB(0)
        , macQueue()
        , queueLength(0)
        , txAttempts(0)
        , bitrate(0)
        , droppedPacket()
        , nicId(-1)
        , ackLength(0)
        , ackMessage(NULL)
        , SeqNrParent()
        , SeqNrChild()
        , macCritDelay(0)
        , TB()
        , delay(0)
        ,latencies(),
        latency(),
        latenciesRaw()
    {}

    virtual ~D2MACWuR();
    /** @brief Initialization of the module and some variables*/
    virtual void initialize(int);
    /** @brief Delete all dynamically allocated objects of the module*/
        virtual void finish();

        /** @brief Handle messages from lower layer */
        virtual void handleLowerMsg(cMessage*);

        /** @brief Handle messages from upper layer */
        virtual void handleUpperMsg(cMessage*);

        /** @brief Handle self messages such as timers */
        virtual void handleSelfMsg(cMessage*);

        /** @brief Handle control messages from lower layer */
        virtual void handleLowerControl(cMessage *msg);

      protected:
        typedef std::list<macpkt_ptr_t> MacQueue;

        /** @name Different tracked statistics.*/
        /*@{*/
        long nbTxWuC;
        long nbTxWuCCSMA;
        long nbRxWuC;
        long nbRxWuCCSMA;
        long nbMissedAcks;
        long nbRecvdAcks;
        long nbDroppedWuC;
        long nbDroppedWuCCSMA;
        long nbTxAcks;
        long nbDuplicates;
        long nbBackoffs;
        double backoffValues;
        /*@}*/

        /** @brief Records general statistics?*/
        bool stats;
        /** @brief Record out put vectors?*/
        bool trace;

        /** @brief MAC states
         * see states diagram.
         */
        enum t_mac_states {
            IDLE_1=1,
            BACKOFF_2,
            CCA_3,
            TRANSMITFRAME_4,
            WAITACK_5,
            WAITSIFS_6,
            TRANSMITACK_7

        };

        enum t_mac_states_pca {
           IDLE_1_PCA=1,
           BACKOFF_2_PCA,
           CCA_3_PCA,
           TRANSMITFRAME_4_PCA,
           WAITACK_5_PCA,
           WAITSIFS_6_PCA,
           TRANSMITACK_7_PCA

        };

        /*************************************************************/
        /****************** TYPES ************************************/
        /*************************************************************/

        enum TYPES{

           WUC_MESSAGE_PCA = 1,
           WUC_MESSAGE_CSMA = 3,

        };


        /** @brief Kinds for timer messages.*/
        enum t_mac_timer {
          TIMER_NULL=0,
          TIMER_BACKOFF,
          TIMER_CCA,
          TIMER_SIFS,
          TIMER_RX_ACK,
        };

        enum t_mac_timer_pca {
          TIMER_NULL_PCA=0,
          TIMER_BACKOFF_PCA,
          TIMER_CCA_PCA,
          TIMER_SIFS_PCA,
          TIMER_RX_ACK_PCA,
        };

        /** @name Pointer for timer messages.*/
        /*@{*/
        cMessage * backoffTimer, * ccaTimer, * sifsTimer, * rxAckTimer;

        cMessage * backoffTimerPCA, * ccaTimerPCA, * sifsTimerPCA, * rxAckTimerPCA, * delaybackoffTimerPCA;
        /*@}*/

        // timers
                cMessage* delayTimer;

        /** @brief MAC state machine events.
         * See state diagram.*/
        enum t_mac_event {
          EV_SEND_REQUEST=1,                   // 1, 11, 20, 21, 22
          EV_TIMER_BACKOFF,                    // 2, 7, 14, 15
          EV_FRAME_TRANSMITTED,                // 4, 19
          EV_ACK_RECEIVED,                     // 5
          EV_ACK_TIMEOUT,                      // 12
          EV_FRAME_RECEIVED,                   // 15, 26
          EV_DUPLICATE_RECEIVED,
          EV_TIMER_SIFS,                       // 17
          EV_BROADCAST_RECEIVED,           // 23, 24
          EV_TIMER_CCA
        };

        //PCA

        enum t_mac_event_pca {
          EV_SEND_REQUEST_PCA=1,                   // 1, 11, 20, 21, 22
          EV_TIMER_BACKOFF_PCA,                    // 2, 7, 14, 15
          EV_FRAME_TRANSMITTED_PCA,                // 4, 19
          EV_ACK_RECEIVED_PCA,                     // 5
          EV_ACK_TIMEOUT_PCA,                      // 12
          EV_FRAME_RECEIVED_PCA,                   // 15, 26
          EV_DUPLICATE_RECEIVED_PCA,
          EV_TIMER_SIFS_PCA,                       // 17
          EV_BROADCAST_RECEIVED_PCA,           // 23, 24
          EV_TIMER_CCA_PCA
        };

        /** @brief Types for frames sent by the CSMA.*/
        enum t_csma_frame_types {
            //DATA,
            WuC,
            ACK
        };

        enum t_mac_carrier_sensed {
          CHANNEL_BUSY=1,
          CHANNEL_FREE
        } ;

        enum t_mac_status {
          STATUS_OK=1,
          STATUS_ERROR,
          STATUS_RX_ERROR,
          STATUS_RX_TIMEOUT,
          STATUS_FRAME_TO_PROCESS,
          STATUS_NO_FRAME_TO_PROCESS,
          STATUS_FRAME_TRANSMITTED
        };

        /** @brief The different back-off methods.*/
        enum backoff_methods {
            /** @brief Constant back-off time.*/
            CONSTANT = 0,
            /** @brief Linear increasing back-off time.*/
            LINEAR,
            /** @brief Exponentially increasing back-off time.*/
            EXPONENTIAL,
        };

        /** @brief keep track of MAC state */
        t_mac_states macState;
        t_mac_states_pca macStatePCA;
        t_mac_status status;

        /** @brief Maximum time between a packet and its ACK
         *
         * Usually this is slightly more then the tx-rx turnaround time
         * The channel should stay clear within this period of time.
         */
        simtime_t sifs;

        /** @brief The amount of time the MAC waits for the ACK of a packet.*/
        simtime_t macAckWaitDuration;

        bool transmissionAttemptInterruptedByRx;
        /** @brief CCA detection time */
        simtime_t ccaDetectionTime;
        /** @brief Time to setup radio from sleep to Rx state */
        simtime_t rxSetupTime;
        /** @brief Time to switch radio from Rx to Tx state */
        simtime_t aTurnaroundTime;
        /** @brief maximum number of backoffs before frame drop */
        int macMaxCSMABackoffs;
        /** @brief maximum number of frame retransmissions without ack */
        unsigned int macMaxFrameRetries;
        /** @brief base time unit for calculating backoff durations */
        simtime_t aUnitBackoffPeriod;
        /** @brief Stores if the MAC expects Acks for Unicast packets.*/
        simtime_t delay;
        simtime_t theLatencyy;
        simtime_t macCritDelay;
        simtime_t now;
        simtime_t theLatency;
        simtime_t elapsedTime;

        simtime_t firstPacketGeneration;
        simtime_t lastPacketReception;
        bool useMACAcks;

        simtime_t backoffTimePCA;
        simtime_t delayBackoff;


        std::map<LAddress::L2Type, cStdDev> latencies;
        cStdDev latency;
        cOutVector latenciesRaw;

        /** @brief Defines the backoff method to be used.*/
        backoff_methods backoffMethod;

        /**
         * @brief Minimum backoff exponent.
         * Only used for exponential backoff method.
         */
        int macMinBE;
        /**
         * @brief Maximum backoff exponent.
         * Only used for exponential backoff method.
         */
        int macMaxBE;

        /** @brief initial contention window size
         * Only used for linear and constant backoff method.*/
        double initialCW;

        /** @brief The power (in mW) to transmit with.*/
        double txPower;

        /** @brief number of backoff performed until now for current frame */
        int NB;

        int TB;
        //int delay;
       // double macCritDelay;

        /** @brief A queue to store packets from upper layer in case another
        packet is still waiting for transmission..*/
        MacQueue macQueue;

        /** @brief length of the queue*/
        unsigned int queueLength;

        /** @brief count the number of tx attempts
         *
         * This holds the number of transmission attempts for the current frame.
         */
        unsigned int txAttempts;

        /** @brief the bit rate at which we transmit */
        double bitrate;

        /** @brief Inspect reasons for dropped packets */
        DroppedPacket droppedPacket;

        /** @brief publish dropped packets nic wide */
        int nicId;

        /** @brief The bit length of the ACK packet.*/
        int ackLength;

        simtime_t arrivalTime;

        int BE;

    protected:
        cStdDev& hostsLatency(const LAddress::L2Type& hostAddress);
        // FSM functions
        void fsmError(t_mac_event event, cMessage *msg);
        void executeMac(t_mac_event event, cMessage *msg);
        void updateStatusIdle(t_mac_event event, cMessage *msg);
        void updateStatusBackoff(t_mac_event event, cMessage *msg);
        void updateStatusCCA(t_mac_event event, cMessage *msg);
        void updateStatusTransmitFrame(t_mac_event event, cMessage *msg);
        void updateStatusWaitAck(t_mac_event event, cMessage *msg);
        void updateStatusSIFS(t_mac_event event, cMessage *msg);
        void updateStatusTransmitAck(t_mac_event event, cMessage *msg);
        void updateStatusNotIdle(cMessage *msg);
        void manageQueue();
        void updateMacState(t_mac_states newMacState);

        void attachSignal(macpkt_ptr_t mac, simtime_t_cref startTime);
        void manageMissingAck(t_mac_event event, cMessage *msg);
        void startTimer(t_mac_timer timer);




        //PCA
        void fsmErrorPCA(t_mac_event_pca event, cMessage *msg);
        void executeMacPCA(t_mac_event_pca event, cMessage *msg);
        void updateStatusIdlePCA(t_mac_event_pca event, cMessage *msg);
        void updateStatusBackoffPCA(t_mac_event_pca event, cMessage *msg);
        void updateStatusCCAPCA(t_mac_event_pca event, cMessage *msg);
        void updateStatusTransmitFramePCA(t_mac_event_pca event, cMessage *msg);
        void updateStatusWaitAckPCA(t_mac_event_pca event, cMessage *msg);
        void updateStatusSIFSPCA(t_mac_event_pca event, cMessage *msg);
        void updateStatusTransmitAckPCA(t_mac_event_pca event, cMessage *msg);
        void updateStatusNotIdlePCA(cMessage *msg);

        void updateMacStatePCA(t_mac_states_pca newMacState);
        void manageMissingAckPCA(t_mac_event_pca event, cMessage *msg);
        void startTimerPCA(t_mac_timer_pca timer);
        void manageQueuePCA();


        virtual simtime_t scheduleBackoff();

        virtual cPacket *decapsMsg(macpkt_ptr_t macPkt);
        macpkt_ptr_t ackMessage;

        //sequence number for sending, map for the general case with more senders
        //also in initialisation phase multiple potential parents
        std::map<LAddress::L2Type, unsigned long> SeqNrParent; //parent -> sequence number

        //sequence numbers for receiving
        std::map<LAddress::L2Type, unsigned long> SeqNrChild; //child -> sequence number

    private:
        /** @brief Copy constructor is not allowed.
         */
        D2MACWuR(const D2MACWuR&);
        /** @brief Assignment operator is not allowed.
         */
        D2MACWuR& operator=(const D2MACWuR&);


};

#endif
