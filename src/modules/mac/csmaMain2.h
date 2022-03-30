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

#ifndef __MIXIM_CSMAMAIN2_H_
#define __MIXIM_CSMAMAIN2_H_

#include <omnetpp.h>

#include <string>
#include <sstream>
#include <vector>
#include <list>

#include "MiXiMDefs.h"
#include "BaseMacLayer.h"
#include "DroppedPacket.h"

class MacPkt;

/**
 * TODO - Generated class
 */
class MIXIM_API csmaMain2 : public BaseMacLayer
{
public:
    csmaMain2()
        : BaseMacLayer()
        , nbTxFrames(0)
        , nbTxFramesCSMA(0)
        , nbRxFrames(0)
        , nbRxFramesCSMA(0)
        , nbMissedAcks(0)
        , nbMissedAcksCSMA(0)
        , nbRecvdAcks(0)
        , nbRecvdAcksCSMA(0)
        , nbDroppedFrames(0)
        , nbDroppedFramesCSMA(0)
        , nbTxAcks(0)
        , nbTxAcksCSMA(0)
        , nbDuplicates(0)
        , nbBackoffs(0)
        , backoffValues(0)
        , stats(false)
        , trace(false)
        , backoffTimer(NULL), ccaTimer(NULL), sifsTimer(NULL), rxAckTimer(NULL)
        , backoffTimerCSMA(NULL), ccaTimerCSMA(NULL), sifsTimerCSMA(NULL), rxAckTimerCSMA(NULL)
        , macState(IDLE_1)
        , macStateCSMA(IDLE_1_CSMA)
        , status(STATUS_OK)
        , sifs()
        , macAckWaitDuration()
        , transmissionAttemptInterruptedByRx(false)
        , ccaDetectionTime()
        , rxSetupTime()
        , aTurnaroundTime()
        , delayAck()
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
    {}

    virtual ~csmaMain2();
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
        /** @brief Handle control messages from lower layer */
        virtual void handleUpperControl(cMessage *msg);

protected:
    typedef std::list<macpkt_ptr_t> MacQueue;

    /** @name Different tracked statistics.*/
    /*@{*/
    long nbTxFrames;
    long nbTxFramesCSMA;
    long nbRxFrames;
    long nbRxFramesCSMA;
    long nbMissedAcks;
    long nbMissedAcksCSMA;
    long nbRecvdAcks;
    long nbRecvdAcksCSMA;
    long nbDroppedFrames;
    long nbDroppedFramesCSMA;
    long nbTxAcks;
    long nbTxAcksCSMA;
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
    enum t_mac_states_csma {
           IDLE_1_CSMA=1,
           BACKOFF_2_CSMA,
           CCA_3_CSMA,
           TRANSMITFRAME_4_CSMA,
           WAITACK_5_CSMA,
           WAITSIFS_6_CSMA,
           TRANSMITACK_7_CSMA

       };

    /*************************************************************/
    /****************** TYPES ************************************/
    /*************************************************************/

    enum TYPES{
         WUC_MESSAGE = 1,
         DATA_MESSAGE = 2,
         DATA_MESSAGE_CSMA = 4,
    };

    /** @brief Kinds for timer messages.*/
    enum t_mac_timer {
      TIMER_NULL=0,
      TIMER_BACKOFF,
      TIMER_CCA,
      TIMER_SIFS,
      TIMER_RX_ACK,
    };

    enum t_mac_timer_csma {
          TIMER_NULL_CSMA=0,
          TIMER_BACKOFF_CSMA,
          TIMER_CCA_CSMA,
          TIMER_SIFS_CSMA,
          TIMER_RX_ACK_CSMA,
        };

    /** @name Pointer for timer messages.*/
    /*@{*/
    cMessage * backoffTimer, * ccaTimer, * sifsTimer, * rxAckTimer;
    cMessage * backoffTimerCSMA, * ccaTimerCSMA, * sifsTimerCSMA, * rxAckTimerCSMA;
    /*@}*/

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
    enum t_mac_event_csma {
      EV_SEND_REQUEST_CSMA=1,                   // 1, 11, 20, 21, 22
      EV_TIMER_BACKOFF_CSMA,                    // 2, 7, 14, 15
      EV_FRAME_TRANSMITTED_CSMA,                // 4, 19
      EV_ACK_RECEIVED_CSMA,                     // 5
      EV_ACK_TIMEOUT_CSMA,                      // 12
      EV_FRAME_RECEIVED_CSMA,                   // 15, 26
      EV_DUPLICATE_RECEIVED_CSMA,
      EV_TIMER_SIFS_CSMA,                       // 17
      EV_BROADCAST_RECEIVED_CSMA,           // 23, 24
      EV_TIMER_CCA_CSMA
        };

public:

    /** @brief Types for frames sent by the CSMA.*/
    enum t_csma_frame_types {
        DATA,
        ACK = 9,
        ACK_CSMA = 17
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
    t_mac_states_csma macStateCSMA;
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
    simtime_t delayAck;
    /** @brief maximum number of backoffs before frame drop */
    int macMaxCSMABackoffs;
    /** @brief maximum number of frame retransmissions without ack */
    unsigned int macMaxFrameRetries;
    /** @brief base time unit for calculating backoff durations */
    simtime_t aUnitBackoffPeriod;
    /** @brief Stores if the MAC expects Acks for Unicast packets.*/
    bool useMACAcks;

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

protected:
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

    void fsmErrorCSMA(t_mac_event_csma event, cMessage *msg);
    void executeMacCSMA(t_mac_event_csma event, cMessage *msg);
    void updateStatusIdleCSMA(t_mac_event_csma event, cMessage *msg);
        //void updateStatusBackoff(t_mac_event event, cMessage *msg);
    void updateStatusCCACSMA(t_mac_event_csma event, cMessage *msg);
    void updateStatusTransmitFrameCSMA(t_mac_event_csma event, cMessage *msg);
    void updateStatusWaitAckCSMA(t_mac_event_csma event, cMessage *msg);
    void updateStatusSIFSCSMA(t_mac_event_csma event, cMessage *msg);
    void updateStatusTransmitAckCSMA(t_mac_event_csma event, cMessage *msg);
    void updateStatusNotIdleCSMA(cMessage *msg);
    void manageQueueCSMA();
    void updateMacStateCSMA(t_mac_states_csma newMacState);
    void manageMissingAckCSMA(t_mac_event_csma event, cMessage *msg);
    void startTimerCSMA(t_mac_timer_csma timer);

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
    csmaMain2(const csmaMain2&);
    /** @brief Assignment operator is not allowed.
     */
    csmaMain2& operator=(const csmaMain2&);
};

#endif
