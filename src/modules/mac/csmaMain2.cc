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

#include "csmaMain2.h"
#include <cassert>

#include "FWMath.h"
#include "BaseDecider.h"
#include "BaseArp.h"
#include "BasePhyLayer.h"
#include "BaseConnectionManager.h"
#include "FindModule.h"
#include "MacPkt_m.h"
#include "IControl.h"
#include <BasePhyLayer.h>
//#include "SensorAppLayerWuR.h"
//#include "DualRadio.h"
#include "DualRadioPCA.h"
#include "Decider802154Narrow.h"

Define_Module(csmaMain2);

void csmaMain2::initialize(int stage)
{
    BaseMacLayer::initialize(stage);

        if (stage == 0) {
            BaseLayer::catDroppedPacketSignal.initialize();

            useMACAcks = par("useMACAcks").boolValue();
            queueLength = par("queueLength");
            sifs = par("sifs");
            transmissionAttemptInterruptedByRx = false;
            nbTxFrames = 0;
            nbTxFramesCSMA = 0;
            nbRxFrames = 0;
            nbRxFramesCSMA = 0;
            nbMissedAcks = 0;
            nbMissedAcksCSMA = 0;
            nbTxAcks = 0;
            nbTxAcksCSMA = 0;
            nbRecvdAcks = 0;
            nbRecvdAcksCSMA = 0;
            nbDroppedFrames = 0;
            nbDroppedFramesCSMA = 0;
            nbDuplicates = 0;
            nbBackoffs = 0;
            backoffValues = 0;
            stats = par("stats");
            trace = par("trace");
            macMaxCSMABackoffs = par("macMaxCSMABackoffs");
            macMaxFrameRetries = par("macMaxFrameRetries");
            macAckWaitDuration = par("macAckWaitDuration").doubleValue();
            aUnitBackoffPeriod = par("aUnitBackoffPeriod").doubleValue();
            ccaDetectionTime = par("ccaDetectionTime").doubleValue();
            rxSetupTime = par("rxSetupTime").doubleValue();
            aTurnaroundTime = par("aTurnaroundTime").doubleValue();
            delayAck = par("delayAck").doubleValue();
            bitrate = par("bitrate");
            ackLength = par("ackLength");
            ackMessage = NULL;

            //init parameters for backoff method
            std::string backoffMethodStr = par("backoffMethod").stdstringValue();
            if(backoffMethodStr == "exponential") {
                backoffMethod = EXPONENTIAL;
                macMinBE = par("macMinBE");
                macMaxBE = par("macMaxBE");
            }
            else {
                if(backoffMethodStr == "linear") {
                    backoffMethod = LINEAR;
                }
                else if (backoffMethodStr == "constant") {
                    backoffMethod = CONSTANT;
                }
                else {
                    error("Unknown backoff method \"%s\".\
                           Use \"constant\", \"linear\" or \"\
                           \"exponential\".", backoffMethodStr.c_str());
                }
                initialCW = par("contentionWindow");
            }
            NB = 0;

            txPower = par("txPower").doubleValue();

            droppedPacket.setReason(DroppedPacket::NONE);
            nicId = getNic()->getId();

            // initialize the timers
            backoffTimer = new cMessage("timer-backoff");
            ccaTimer = new cMessage("timer-cca");
            sifsTimer = new cMessage("timer-sifs");
            rxAckTimer = new cMessage("timer-rxAck");
            backoffTimerCSMA = new cMessage("timer-backoff-CSMA");
            ccaTimerCSMA = new cMessage("timer-cca-csma");
            sifsTimerCSMA = new cMessage("timer-sifs-csma");
            rxAckTimerCSMA = new cMessage("timer-rxAck-csma");
            macState = IDLE_1;
            macStateCSMA = IDLE_1_CSMA;
            txAttempts = 0;

            //check parameters for consistency
            cModule* phyModule = FindModule<BasePhyLayer*>::findSubModule(getNic());

            //aTurnaroundTime should match (be equal or bigger) the RX to TX
            //switching time of the radio
            if(phyModule->hasPar("timeRXToTX")) {
                simtime_t rxToTx = phyModule->par("timeRXToTX").doubleValue();
                if( rxToTx > aTurnaroundTime)
                {
                    opp_warning("Parameter \"aTurnaroundTime\" (%f) does not match"
                                " the radios RX to TX switching time (%f)! It"
                                " should be equal or bigger",
                                SIMTIME_DBL(aTurnaroundTime), SIMTIME_DBL(rxToTx));
                }
            }

        } else if(stage == 1) {
            phy->setRadioState(MiximRadio::SLEEP);
            BaseConnectionManager* cc = getConnectionManager();

            if(cc->hasPar("pMax") && txPower > cc->par("pMax").doubleValue())
                opp_error("TranmitterPower can't be bigger than pMax in ConnectionManager! "
                          "Please adjust your omnetpp.ini file accordingly");

            EV << "queueLength = " << queueLength
            << " bitrate = " << bitrate
            << " backoff method = " << par("backoffMethod").stringValue() << endl;

            EV << "finished csma init stage 1." << endl;
        }
    }

    void csmaMain2::finish() {
        if (stats) {
            recordScalar("nbTxFrames", nbTxFrames);
            recordScalar("nbRxFrames", nbRxFrames);
            recordScalar("nbDroppedFrames", nbDroppedFrames);
            recordScalar("nbMissedAcks", nbMissedAcks);
            recordScalar("nbRecvdAcks", nbRecvdAcks);
            recordScalar("nbTxAcks", nbTxAcks);
            recordScalar("nbDuplicates", nbDuplicates);
            recordScalar("nbTxFramesCSMA", nbTxFramesCSMA);
            recordScalar("nbRxFramesCSMA", nbRxFramesCSMA);
            recordScalar("nbDroppedFramesCSMA", nbDroppedFramesCSMA);
            recordScalar("nbMissedAcksCSMA", nbMissedAcksCSMA);
            recordScalar("nbRecvdAcksCSMA", nbRecvdAcksCSMA);
            recordScalar("nbTxAcksCSMA", nbTxAcksCSMA);
            if (nbBackoffs > 0) {
                recordScalar("meanBackoff", backoffValues / nbBackoffs);
            } else {
                recordScalar("meanBackoff", 0);
            }
            recordScalar("nbBackoffs", nbBackoffs);
            recordScalar("backoffDurations", backoffValues);
        }
        BaseMacLayer::finish();
    }

    csmaMain2::~csmaMain2() {
        cancelAndDelete(backoffTimer);
        cancelAndDelete(ccaTimer);
        cancelAndDelete(sifsTimer);
        cancelAndDelete(rxAckTimer);
        cancelAndDelete(backoffTimerCSMA);
        cancelAndDelete(ccaTimerCSMA);
        cancelAndDelete(sifsTimerCSMA);
        cancelAndDelete(rxAckTimerCSMA);
        if (ackMessage)
            delete ackMessage;
        MacQueue::iterator it;
        for (it = macQueue.begin(); it != macQueue.end(); ++it) {
            delete (*it);
        }
    }

    /**
     * Encapsulates the message to be transmitted and pass it on
     * to the FSM main method for further processing.
     */
    void csmaMain2::handleUpperMsg(cMessage *msg) {

           EV<< "Received frame name= " << msg->getName() << endl;
            EV << "Message Name=" << msg->getName() << " and Message Type= " <<  msg->getKind() << "\n";

            if (msg->getKind() == DATA_MESSAGE){
            EV <<"ppkt received from NicController to MAC" << endl;
            EV << "Message Name=" << msg->getName() << " and Message Type= " <<  msg->getKind() << "\n";

        //macpkt_ptr_tmacPkt = encapsMsg(msg);
        macpkt_ptr_t macPkt = new MacPkt(msg->getName(), msg->getKind());
        macPkt->setBitLength(headerLength);
        cObject *const cInfo = msg->removeControlInfo();
        EV<<"CSMA received a message from upper layer, name is " << msg->getName() <<", CInfo removed, mac addr="<< getUpperDestinationFromControlInfo(cInfo) << endl;
        LAddress::L2Type dest = getUpperDestinationFromControlInfo(cInfo);
        macPkt->setDestAddr(dest);
        delete cInfo;
        macPkt->setSrcAddr(myMacAddr);

        if(useMACAcks) {
            if(SeqNrParent.find(dest) == SeqNrParent.end()) {
                //no record of current parent -> add next sequence number to map
                SeqNrParent[dest] = 1;
                macPkt->setSequenceId(0);
                EV << "Adding a new parent to the map of Sequence numbers:" << dest << endl;
            }
            else {
                macPkt->setSequenceId(SeqNrParent[dest]);
                EV << "Packet send with sequence number = " << SeqNrParent[dest] << endl;
                SeqNrParent[dest]++;
            }
        }

        //RadioAccNoise3PhyControlInfo *pco = new RadioAccNoise3PhyControlInfo(bitrate);
        //macPkt->setControlInfo(pco);
        assert(static_cast<cPacket*>(msg));
        macPkt->encapsulate(static_cast<cPacket*>(msg));
        EV <<"pkt encapsulated, length: " << macPkt->getBitLength() << "\n";
        executeMac(EV_SEND_REQUEST, macPkt);
    }

            else if (msg->getKind() ==DATA_MESSAGE_CSMA){
                    EV <<"ppkt received from NicController to MAC" << endl;
                    EV << "Message Name=" << msg->getName() << " and Message Type= " <<  msg->getKind() << "\n";

                //macpkt_ptr_tmacPkt = encapsMsg(msg);
                macpkt_ptr_t macPkt = new MacPkt(msg->getName(), msg->getKind());
                macPkt->setBitLength(headerLength);
                cObject *const cInfo = msg->removeControlInfo();
                EV<<"CSMA received a message from upper layer, name is " << msg->getName() <<", CInfo removed, mac addr="<< getUpperDestinationFromControlInfo(cInfo) << endl;
                LAddress::L2Type dest = getUpperDestinationFromControlInfo(cInfo);
                macPkt->setDestAddr(dest);
                delete cInfo;
                macPkt->setSrcAddr(myMacAddr);

                if(useMACAcks) {
                    if(SeqNrParent.find(dest) == SeqNrParent.end()) {
                        //no record of current parent -> add next sequence number to map
                        SeqNrParent[dest] = 1;
                        macPkt->setSequenceId(0);
                        EV << "Adding a new parent to the map of Sequence numbers:" << dest << endl;
                    }
                    else {
                        macPkt->setSequenceId(SeqNrParent[dest]);
                        EV << "Packet send with sequence number = " << SeqNrParent[dest] << endl;
                        SeqNrParent[dest]++;
                    }
                }

                //RadioAccNoise3PhyControlInfo *pco = new RadioAccNoise3PhyControlInfo(bitrate);
                //macPkt->setControlInfo(pco);
                assert(static_cast<cPacket*>(msg));
                macPkt->encapsulate(static_cast<cPacket*>(msg));
                EV <<"pkt encapsulated, length: " << macPkt->getBitLength() << "\n";
                executeMacCSMA(EV_SEND_REQUEST_CSMA, macPkt);
            }
        else{
                delete(msg);
            }
    }

    void csmaMain2::updateStatusIdle(t_mac_event event, cMessage *msg) {
        switch (event) {
        case EV_SEND_REQUEST:
            if (macQueue.size() <= queueLength) {
                macQueue.push_back(static_cast<macpkt_ptr_t> (msg));
                EV<<"(1) FSM State IDLE_1, EV_SEND_REQUEST and [TxBuff avail]: startTimerBackOff -> BACKOFF." << endl;
                updateMacState(CCA_3);
                NB = 0;
                //BE = macMinBE;
                startTimer(TIMER_CCA);
            } else {
                // queue is full, message has to be deleted
                EV << "(12) FSM State IDLE_1, EV_SEND_REQUEST and [TxBuff not avail]: dropping packet -> IDLE." << endl;
                msg->setName("MAC ERROR");
                msg->setKind(PACKET_DROPPED);
                sendControlUp(msg);
                droppedPacket.setReason(DroppedPacket::QUEUE);
                emit(BaseLayer::catDroppedPacketSignal, &droppedPacket);
                updateMacState(IDLE_1);
                phy->setRadioState(MiximRadio::SLEEP);
            }
            break;
        case EV_DUPLICATE_RECEIVED:
            EV << "(15) FSM State IDLE_1, EV_DUPLICATE_RECEIVED: setting up radio tx -> WAITSIFS." << endl;
            //sendUp(decapsMsg(static_cast<MacSeqPkt *>(msg)));
            delete msg;

            if(useMACAcks) {
                phy->setRadioState(MiximRadio::TX);
                updateMacState(WAITSIFS_6);
                startTimer(TIMER_SIFS);
            }
            break;

        case EV_FRAME_RECEIVED:
            EV << "(15) FSM State IDLE_1, EV_FRAME_RECEIVED: setting up radio tx -> WAITSIFS." << endl;
            sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
            nbRxFrames++;
            delete msg;

            if(useMACAcks) {
                phy->setRadioState(MiximRadio::TX);
                updateMacState(WAITSIFS_6);
                startTimer(TIMER_SIFS);
            }
            break;

        case EV_BROADCAST_RECEIVED:
            EV << "(23) FSM State IDLE_1, EV_BROADCAST_RECEIVED: Nothing to do." << endl;
            nbRxFrames++;
            sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
            delete msg;
            break;
        default:
            fsmError(event, msg);
            break;
        }
    }


    void csmaMain2::attachSignal(macpkt_ptr_t mac, simtime_t_cref startTime) {
        simtime_t duration = (mac->getBitLength() + phyHeaderLength)/bitrate;
        setDownControlInfo(mac, createSignal(startTime, duration, txPower, bitrate));
    }

    void csmaMain2::updateStatusCCA(t_mac_event event, cMessage *msg) {
        switch (event) {
        case EV_TIMER_CCA:
        {
            EV<< "(25) FSM State CCA_3, EV_TIMER_CCA" << endl;
           // bool isIdle = phy->getChannelState().isIdle();
            //if(isIdle) {
            EV << "Message Name=" << msg->getName() << " and Message Type= " <<  msg->getKind() << "\n";
                EV << "(3) FSM State CCA_3, EV_TIMER_CCA, [Channel Idle]: -> TRANSMITFRAME_4." << endl;
                updateMacState(TRANSMITFRAME_4);
                phy->setRadioState(MiximRadio::TX);
                macpkt_ptr_t mac = check_and_cast<macpkt_ptr_t>(macQueue.front()->dup());
                attachSignal(mac, simTime()+aTurnaroundTime);
                //sendDown(msg);
                // give time for the radio to be in Tx state before transmitting
                sendDelayed(mac, aTurnaroundTime, lowerLayerOut);
                nbTxFrames++;
            break;
        }
        case EV_DUPLICATE_RECEIVED:
            debugEV << "(26) FSM State CCA_3, EV_DUPLICATE_RECEIVED:";
            if(useMACAcks) {
                debugEV << " setting up radio tx -> WAITSIFS." << endl;
                // suspend current transmission attempt,
                // transmit ack,
                // and resume transmission when entering manageQueue()
                transmissionAttemptInterruptedByRx = true;
                cancelEvent(ccaTimer);

                phy->setRadioState(MiximRadio::TX);
                updateMacState(WAITSIFS_6);
                startTimer(TIMER_SIFS);
            } else {
                debugEV << " Nothing to do." << endl;
            }
            //sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
            delete msg;
            break;

        case EV_FRAME_RECEIVED:
            EV << "(26) FSM State CCA_3, EV_FRAME_RECEIVED:";
            if(useMACAcks) {
                EV << " setting up radio tx -> WAITSIFS." << endl;
                // suspend current transmission attempt,
                // transmit ack,
                // and resume transmission when entering manageQueue()
                transmissionAttemptInterruptedByRx = true;
                cancelEvent(ccaTimer);
                phy->setRadioState(MiximRadio::TX);
                updateMacState(WAITSIFS_6);
                startTimer(TIMER_SIFS);
            } else {
                debugEV << " Nothing to do." << endl;
            }
            sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
            delete msg;
            break;
        case EV_BROADCAST_RECEIVED:
            debugEV << "(24) FSM State BACKOFF, EV_BROADCAST_RECEIVED:"
            << " Nothing to do." << endl;
            sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
            delete msg;
            break;
        default:
            fsmError(event, msg);
            break;
        }
    }

    void csmaMain2::updateStatusTransmitFrame(t_mac_event event, cMessage *msg) {
        if (event == EV_FRAME_TRANSMITTED) {
            //    delete msg;
            macpkt_ptr_t packet = macQueue.front();
            phy->setRadioState(MiximRadio::RX);

            bool expectAck = useMACAcks;
            if (!LAddress::isL2Broadcast(packet->getDestAddr())) {
                //unicast
                EV << "(4) FSM State TRANSMITFRAME_4, "
                   << "EV_FRAME_TRANSMITTED [Unicast]: ";
            } else {
                //broadcast
                EV << "(27) FSM State TRANSMITFRAME_4, EV_FRAME_TRANSMITTED "
                   << " [Broadcast]";
                expectAck = false;
            }

            if(expectAck) {
                EV << "RadioSetupRx -> WAITACK." << endl;
                updateMacState(WAITACK_5);
                startTimer(TIMER_RX_ACK);
            } else {
                EV << ": RadioSetupRx, manageQueue..." << endl;
                macQueue.pop_front();
                delete packet;
                manageQueue();
            }
        } else {
            fsmError(event, msg);
        }
    }

    void csmaMain2::updateStatusWaitAck(t_mac_event event, cMessage *msg) {
        assert(useMACAcks);

        cMessage * mac;
        switch (event) {
        case EV_ACK_RECEIVED:
            EV<< "(5) FSM State WAITACK_5, EV_ACK_RECEIVED: "
            << " ProcessAck, manageQueue..." << endl;
            if(rxAckTimer->isScheduled())
            cancelEvent(rxAckTimer);
            mac = static_cast<cMessage *>(macQueue.front());
            macQueue.pop_front();
            txAttempts = 0;
            sendUp(msg);
            mac->setName("MAC SUCCESS");
            mac->setKind(DualRadioPCA::TRANSMISSION_OVER);
            sendControlUp(mac);
           // sendUp(msg);
           // sendDelayed(msg, delayAck, upperLayerOut);
           // sendUp(msg);
            //sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
           // delete msg;
            manageQueue();
            break;
        case EV_ACK_TIMEOUT:
            EV << "(12) FSM State WAITACK_5, EV_ACK_TIMEOUT:"
            << " incrementCounter/dropPacket, manageQueue..." << endl;
            manageMissingAck(event, msg);
            mac = static_cast<cMessage *>(macQueue.front());
            macQueue.pop_front();
            mac->setName("RX_TIMEOUT_MSG_TYPE");
            mac->setKind(RX_TIMEOUT_MSG_TYPE);
            sendControlUp(mac);
            break;
        case EV_BROADCAST_RECEIVED:
        case EV_FRAME_RECEIVED:
            sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
            break;
        case EV_DUPLICATE_RECEIVED:
            debugEV << "Error ! Received a frame during SIFS !" << endl;
            delete msg;
            break;
        default:
            fsmError(event, msg);
            break;
        }

    }

    void csmaMain2::manageMissingAck(t_mac_event /*event*/, cMessage */*msg*/) {
        if (txAttempts < macMaxFrameRetries + 1) {
            // increment counter
            txAttempts++;
            EV<< "I will retransmit this packet (I already tried "
            << txAttempts << " times)." << endl;
        } else {
            // drop packet
            debugEV << "Packet was transmitted " << txAttempts
            << " times and I never got an Ack. I drop the packet." << endl;
            cMessage * mac = macQueue.front();
            macQueue.pop_front();
            txAttempts = 0;
            mac->setName("MAC ERROR");
            mac->setKind(PACKET_DROPPED);
            sendControlUp(mac);
            phy->setRadioState(MiximRadio::SLEEP);
        }
        manageQueue();
    }
    void csmaMain2::updateStatusSIFS(t_mac_event event, cMessage *msg) {
        assert(useMACAcks);

        switch (event) {
        case EV_TIMER_SIFS:
            EV<< "(17) FSM State WAITSIFS_6, EV_TIMER_SIFS:"
            << " sendAck -> TRANSMITACK." << endl;
            //updateMacState(TRANSMITACK_7);
            updateMacState(IDLE_1);
            attachSignal(ackMessage, simTime());
            sendDown(ackMessage);
            sendControlUp(new cMessage("ACK_SENT", DualRadioPCA::ACK_SENT));
            nbTxAcks++;
            //      sendDelayed(ackMessage, aTurnaroundTime, lowerLayerOut);
            ackMessage = NULL;
            break;
        case EV_TIMER_BACKOFF:
            // Backoff timer has expired while receiving a frame. Restart it
            // and stay here.
            EV << "(16) FSM State WAITSIFS_6, EV_TIMER_BACKOFF. "
            << "Restart backoff timer and don't move." << endl;
            //startTimer(TIMER_BACKOFF);
            break;
        case EV_BROADCAST_RECEIVED:
        case EV_FRAME_RECEIVED:
            EV << "Error ! Received a frame during SIFS !" << endl;
            sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
            delete msg;
            break;
        default:
            fsmError(event, msg);
            break;
        }
    }

    void csmaMain2::updateStatusTransmitAck(t_mac_event event, cMessage *msg) {
        assert(useMACAcks);

        if (event == EV_FRAME_TRANSMITTED) {
            EV<< "(19) FSM State TRANSMITACK_7, EV_FRAME_TRANSMITTED:"
            << " ->manageQueue." << endl;
            phy->setRadioState(MiximRadio::RX);
            //      delete msg;
            manageQueue();
        } else {
            fsmError(event, msg);
        }
    }

    void csmaMain2::updateStatusNotIdle(cMessage *msg) {
        EV<< "(20) FSM State NOT IDLE, EV_SEND_REQUEST. Is a TxBuffer available ?" << endl;
        if (macQueue.size() <= queueLength) {
            macQueue.push_back(static_cast<macpkt_ptr_t>(msg));
            EV << "(21) FSM State NOT IDLE, EV_SEND_REQUEST"
            <<" and [TxBuff avail]: enqueue packet and don't move." << endl;
            phy->setRadioState(MiximRadio::SLEEP);
        } else {
            // queue is full, message has to be deleted
            EV << "(22) FSM State NOT IDLE, EV_SEND_REQUEST"
            << " and [TxBuff not avail]: dropping packet and don't move."
            << endl;
            msg->setName("MAC ERROR");
            msg->setKind(PACKET_DROPPED);
            sendControlUp(msg);
            droppedPacket.setReason(DroppedPacket::QUEUE);
            emit(BaseLayer::catDroppedPacketSignal, &droppedPacket);
            phy->setRadioState(MiximRadio::SLEEP);
        }

    }
    /**
     * Updates state machine.
     */
    void csmaMain2::executeMac(t_mac_event event, cMessage *msg) {
        EV<< "In executeMac" << endl;
        if(macState != IDLE_1 && event == EV_SEND_REQUEST) {
            updateStatusNotIdle(msg);
        } else {
            switch(macState) {
            case IDLE_1:
                updateStatusIdle(event, msg);
                break;
            case CCA_3:
                updateStatusCCA(event, msg);
                break;
            case TRANSMITFRAME_4:
                updateStatusTransmitFrame(event, msg);
                break;
            case WAITACK_5:
                updateStatusWaitAck(event, msg);
                break;
            case WAITSIFS_6:
                updateStatusSIFS(event, msg);
                break;
            case TRANSMITACK_7:
                updateStatusTransmitAck(event, msg);
                break;
            default:
                EV << "Error in CSMA FSM: an unknown state has been reached. macState=" << macState << endl;
                break;
            }
        }
    }


    void csmaMain2::manageQueue() {
        if (macQueue.size() != 0) {
            EV<< "(manageQueue) there are " << macQueue.size() << " packets to send, entering backoff wait state." << endl;
            if( transmissionAttemptInterruptedByRx) {
                // resume a transmission cycle which was interrupted by
                // a frame reception during CCA check
                transmissionAttemptInterruptedByRx = false;
            } else {
                // initialize counters if we start a new transmission
                // cycle from zero
                NB = 0;
                //BE = macMinBE;
            }

        } else {
            EV << "(manageQueue) no packets to send, entering IDLE state." << endl;
            updateMacState(IDLE_1);
            phy->setRadioState(MiximRadio::SLEEP);
        }
    }

    void csmaMain2::updateMacState(t_mac_states newMacState) {
        macState = newMacState;
    }

    /*
     * Called by the FSM machine when an unknown transition is requested.
     */
    void csmaMain2::fsmError(t_mac_event event, cMessage *msg) {
        EV<< "FSM Error ! In state " << macState << ", received unknown event:" << event << "." << endl;
        if (msg != NULL)
        delete msg;
    }

    void csmaMain2::startTimer(t_mac_timer timer) {
        if (timer == TIMER_BACKOFF) {
            scheduleAt(scheduleBackoff(), backoffTimer);
        } else if (timer == TIMER_CCA) {
            simtime_t ccaTime = rxSetupTime + ccaDetectionTime;
            debugEV<< "(startTimer) ccaTimer value=" << ccaTime
            << "(rxSetupTime,ccaDetectionTime:" << rxSetupTime
            << "," << ccaDetectionTime <<")." << endl;
            scheduleAt(simTime()+rxSetupTime+ccaDetectionTime, ccaTimer);
        } else if (timer==TIMER_SIFS) {
            assert(useMACAcks);
            debugEV << "(startTimer) sifsTimer value=" << sifs << endl;
            scheduleAt(simTime()+sifs, sifsTimer);
        } else if (timer==TIMER_RX_ACK) {
            assert(useMACAcks);
            debugEV << "(startTimer) rxAckTimer value=" << macAckWaitDuration << endl;
            scheduleAt(simTime()+macAckWaitDuration, rxAckTimer);
        } else {
            EV << "Unknown timer requested to start:" << timer << endl;
        }
    }

    void csmaMain2::updateStatusIdleCSMA(t_mac_event_csma event, cMessage *msg) {
                switch (event) {
                case EV_SEND_REQUEST_CSMA:
                    if (macQueue.size() <= queueLength) {
                        macQueue.push_back(static_cast<macpkt_ptr_t> (msg));
                        EV<<"(1) FSM State IDLE_1, EV_SEND_REQUEST and " << endl;
                        updateMacStateCSMA(CCA_3_CSMA);
                        NB = 0;
                        //BE = macMinBE;
                        startTimerCSMA(TIMER_CCA_CSMA);
                    } else {
                        // queue is full, message has to be deleted
                        EV << "(12) FSM State IDLE_1, EV_SEND_REQUEST and [TxBuff not avail]: dropping packet -> IDLE." << endl;
                        msg->setName("MAC ERROR CSMA");
                        msg->setKind(PACKET_DROPPED_CSMA);
                        sendControlUp(msg);
                        droppedPacket.setReason(DroppedPacket::QUEUE);
                        emit(BaseLayer::catDroppedPacketSignal, &droppedPacket);
                        updateMacStateCSMA(IDLE_1_CSMA);
                        phy->setRadioState(MiximRadio::SLEEP);
                    }
                    break;
                case EV_DUPLICATE_RECEIVED_CSMA:
                    EV << "(15) FSM State IDLE_1, EV_DUPLICATE_RECEIVED: setting up radio tx -> WAITSIFS." << endl;
                    //sendUp(decapsMsg(static_cast<MacSeqPkt *>(msg)));
                    delete msg;

                    if(useMACAcks) {
                        phy->setRadioState(MiximRadio::TX);
                        updateMacStateCSMA(WAITSIFS_6_CSMA);
                        startTimerCSMA(TIMER_SIFS_CSMA);
                    }
                    break;

                case EV_FRAME_RECEIVED_CSMA:
                    EV << "(15) FSM State IDLE_1, EV_FRAME_RECEIVED: setting up radio tx -> WAITSIFS." << endl;
                    sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
                    nbRxFramesCSMA++;
                    delete msg;

                    if(useMACAcks) {
                        phy->setRadioState(MiximRadio::TX);
                        updateMacStateCSMA(WAITSIFS_6_CSMA);
                        startTimerCSMA(TIMER_SIFS_CSMA);
                    }
                    break;

                case EV_BROADCAST_RECEIVED_CSMA:
                    EV << "(23) FSM State IDLE_1, EV_BROADCAST_RECEIVED: Nothing to do." << endl;
                    nbRxFramesCSMA++;
                    sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
                    delete msg;
                    break;
                default:
                    fsmErrorCSMA(event, msg);
                    break;
                }
            }

            void csmaMain2::updateStatusCCACSMA(t_mac_event_csma event, cMessage *msg) {
                switch (event) {
                case EV_TIMER_CCA_CSMA:
                {
                    EV<< "(25) FSM State CCA_3, EV_TIMER_CCA" << endl;
                   // bool isIdle = phy->getChannelState().isIdle();
                    //if(isIdle) {
                    EV << "Message Name=" << msg->getName() << " and Message Type= " <<  msg->getKind() << "\n";
                        EV << "(3) FSM State CCA_3_CSMA, EV_TIMER_CCA, [Channel Idle]: -> TRANSMITFRAME_4_CSMA." << endl;
                        updateMacStateCSMA(TRANSMITFRAME_4_CSMA);
                        phy->setRadioState(MiximRadio::TX);
                        macpkt_ptr_t mac = check_and_cast<macpkt_ptr_t>(macQueue.front()->dup());
                        attachSignal(mac, simTime()+aTurnaroundTime);
                        //sendDown(msg);
                        // give time for the radio to be in Tx state before transmitting
                        sendDelayed(mac, aTurnaroundTime, lowerLayerOut);
                        nbTxFramesCSMA++;

                    break;
                }
                case EV_DUPLICATE_RECEIVED_CSMA:
                    debugEV << "(26) FSM State CCA_3, EV_DUPLICATE_RECEIVED:";
                    if(useMACAcks) {
                        debugEV << " setting up radio tx -> WAITSIFS." << endl;
                        // suspend current transmission attempt,
                        // transmit ack,
                        // and resume transmission when entering manageQueue()
                        transmissionAttemptInterruptedByRx = true;
                        cancelEvent(ccaTimerCSMA);

                        phy->setRadioState(MiximRadio::TX);
                        updateMacStateCSMA(WAITSIFS_6_CSMA);
                        startTimerCSMA(TIMER_SIFS_CSMA);
                    } else {
                        debugEV << " Nothing to do." << endl;
                    }
                    //sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
                    delete msg;
                    break;

                case EV_FRAME_RECEIVED_CSMA:
                    EV << "(26) FSM State CCA_3, EV_FRAME_RECEIVED:";
                    if(useMACAcks) {
                        EV << " setting up radio tx -> WAITSIFS." << endl;
                        // suspend current transmission attempt,
                        // transmit ack,
                        // and resume transmission when entering manageQueue()
                        transmissionAttemptInterruptedByRx = true;
                        cancelEvent(ccaTimerCSMA);
                        phy->setRadioState(MiximRadio::TX);
                        updateMacStateCSMA(WAITSIFS_6_CSMA);
                        startTimerCSMA(TIMER_SIFS_CSMA);
                    } else {
                        debugEV << " Nothing to do." << endl;
                    }
                    sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
                    delete msg;
                    break;
                case EV_BROADCAST_RECEIVED_CSMA:
                    debugEV << "(24) FSM State BACKOFF, EV_BROADCAST_RECEIVED:"
                    << " Nothing to do." << endl;
                    sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
                    delete msg;
                    break;
                default:
                    fsmErrorCSMA(event, msg);
                    break;
                }
            }

            void csmaMain2::updateStatusTransmitFrameCSMA(t_mac_event_csma event, cMessage *msg) {
                if (event == EV_FRAME_TRANSMITTED_CSMA) {
                    //    delete msg;
                    macpkt_ptr_t packet = macQueue.front();
                    phy->setRadioState(MiximRadio::RX);

                    bool expectAck = useMACAcks;
                    if (!LAddress::isL2Broadcast(packet->getDestAddr())) {
                        //unicast
                        EV << "(4) FSM State TRANSMITFRAME_4, "
                           << "EV_FRAME_TRANSMITTED [Unicast]: ";
                    } else {
                        //broadcast
                        EV << "(27) FSM State TRANSMITFRAME_4, EV_FRAME_TRANSMITTED "
                           << " [Broadcast]";
                        expectAck = false;
                    }

                    if(expectAck) {
                        EV << "RadioSetupRx -> WAITACK." << endl;
                        updateMacStateCSMA(WAITACK_5_CSMA);
                        startTimerCSMA(TIMER_RX_ACK_CSMA);
                    } else {
                        EV << ": RadioSetupRx, manageQueue..." << endl;
                        macQueue.pop_front();
                        delete packet;
                        manageQueueCSMA();
                    }
                } else {
                    fsmErrorCSMA(event, msg);
                }
            }

            void csmaMain2::updateStatusWaitAckCSMA(t_mac_event_csma event, cMessage *msg) {
                assert(useMACAcks);

                cMessage * mac;
                switch (event) {
                case EV_ACK_RECEIVED_CSMA:
                    EV<< "(5) FSM State WAITACK_5, EV_ACK_RECEIVED: "
                    << " ProcessAck, manageQueue..." << endl;
                    if(rxAckTimerCSMA->isScheduled())
                    cancelEvent(rxAckTimerCSMA);
                    mac = static_cast<cMessage *>(macQueue.front());
                    macQueue.pop_front();
                    txAttempts = 0;
                    mac->setName("MAC SUCCESS CSMA");
                    mac->setKind(DualRadioPCA::TRANSMISSION_OVER_CSMA);
                    sendControlUp(mac);
                   // sendDelayed(msg, delayAck, upperLayerOut);
                   // sendUp(msg);
                    //sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
                   // delete msg;
                    manageQueueCSMA();
                    break;
                case EV_ACK_TIMEOUT_CSMA:
                    EV << "(12) FSM State WAITACK_5, EV_ACK_TIMEOUT:"
                    << " incrementCounter/dropPacket, manageQueue..." << endl;
                    manageMissingAckCSMA(event, msg);
                    mac = static_cast<cMessage *>(macQueue.front());
                    macQueue.pop_front();
                    mac->setName("RX_TIMEOUT_MSG_TYPE_CSMA");
                    mac->setKind(RX_TIMEOUT_MSG_TYPE_CSMA);
                    sendControlUp(mac);
                    break;
                case EV_BROADCAST_RECEIVED_CSMA:
                case EV_FRAME_RECEIVED_CSMA:
                    sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
                    break;
                case EV_DUPLICATE_RECEIVED_CSMA:
                    debugEV << "Error ! Received a frame during SIFS !" << endl;
                    delete msg;
                    break;
                default:
                    fsmErrorCSMA(event, msg);
                    break;
                }

            }

            void csmaMain2::manageMissingAckCSMA(t_mac_event_csma /*event*/, cMessage */*msg*/) {
                if (txAttempts < macMaxFrameRetries + 1) {
                    // increment counter
                    txAttempts++;
                    EV<< "I will retransmit this packet (I already tried "
                    << txAttempts << " times)." << endl;
                } else {
                    // drop packet
                    debugEV << "Packet was transmitted " << txAttempts
                    << " times and I never got an Ack. I drop the packet." << endl;
                    cMessage * mac = macQueue.front();
                    macQueue.pop_front();
                    txAttempts = 0;
                    mac->setName("MAC ERROR CSMA");
                    mac->setKind(PACKET_DROPPED_CSMA);
                    sendControlUp(mac);
                    phy->setRadioState(MiximRadio::SLEEP);
                }
                manageQueueCSMA();
            }
            void csmaMain2::updateStatusSIFSCSMA(t_mac_event_csma event, cMessage *msg) {
                assert(useMACAcks);

                switch (event) {
                case EV_TIMER_SIFS_CSMA:
                    EV<< "(17) FSM State WAITSIFS_6, EV_TIMER_SIFS:"
                    << " sendAck -> TRANSMITACK CSMA." << endl;
                    //updateMacState(TRANSMITACK_7);
                    updateMacStateCSMA(IDLE_1_CSMA);
                    attachSignal(ackMessage, simTime());
                    sendDown(ackMessage);
                    sendControlUp(new cMessage("ACK_SENT_CSMA", DualRadioPCA::ACK_SENT_CSMA));
                    nbTxAcksCSMA++;
                    //      sendDelayed(ackMessage, aTurnaroundTime, lowerLayerOut);
                    ackMessage = NULL;
                    break;
                case EV_TIMER_BACKOFF_CSMA:
                    // Backoff timer has expired while receiving a frame. Restart it
                    // and stay here.
                    EV << "(16) FSM State WAITSIFS_6, EV_TIMER_BACKOFF. "
                    << "Restart backoff timer and don't move." << endl;
                    //startTimer(TIMER_BACKOFF);
                    break;
                case EV_BROADCAST_RECEIVED_CSMA:
                case EV_FRAME_RECEIVED_CSMA:
                    EV << "Error ! Received a frame during SIFS !" << endl;
                    sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
                    delete msg;
                    break;
                default:
                    fsmErrorCSMA(event, msg);
                    break;
                }
            }

            void csmaMain2::updateStatusTransmitAckCSMA(t_mac_event_csma event, cMessage *msg) {
                assert(useMACAcks);

                if (event == EV_FRAME_TRANSMITTED_CSMA) {
                    EV<< "(19) FSM State TRANSMITACK_7, EV_FRAME_TRANSMITTED_CSMA:"
                    << " ->manageQueue." << endl;
                    phy->setRadioState(MiximRadio::RX);
                    //      delete msg;
                    manageQueueCSMA();
                } else {
                    fsmErrorCSMA(event, msg);
                }
            }

            void csmaMain2::updateStatusNotIdleCSMA(cMessage *msg) {
                EV<< "(20) FSM State NOT IDLE, EV_SEND_REQUEST. Is a TxBuffer available ?" << endl;
                if (macQueue.size() <= queueLength) {
                    macQueue.push_back(static_cast<macpkt_ptr_t>(msg));
                    EV << "(21) FSM State NOT IDLE, EV_SEND_REQUEST"
                    <<" and [TxBuff avail]: enqueue packet and don't move." << endl;
                    phy->setRadioState(MiximRadio::SLEEP);
                } else {
                    // queue is full, message has to be deleted
                    EV << "(22) FSM State NOT IDLE, EV_SEND_REQUEST"
                    << " and [TxBuff not avail]: dropping packet and don't move."
                    << endl;
                    msg->setName("MAC ERROR CSMA");
                    msg->setKind(PACKET_DROPPED_CSMA);
                    sendControlUp(msg);
                    droppedPacket.setReason(DroppedPacket::QUEUE);
                    emit(BaseLayer::catDroppedPacketSignal, &droppedPacket);
                    phy->setRadioState(MiximRadio::SLEEP);
                }

            }
            /**
             * Updates state machine.
             */
            void csmaMain2::executeMacCSMA(t_mac_event_csma event, cMessage *msg) {
                EV<< "In executeMac" << endl;
                if(macStateCSMA != IDLE_1_CSMA && event == EV_SEND_REQUEST_CSMA) {
                    updateStatusNotIdleCSMA(msg);
                } else {
                    switch(macStateCSMA) {
                    case IDLE_1_CSMA:
                        updateStatusIdleCSMA(event, msg);
                        break;
                    case CCA_3_CSMA:
                        updateStatusCCACSMA(event, msg);
                        break;
                    case TRANSMITFRAME_4_CSMA:
                        updateStatusTransmitFrameCSMA(event, msg);
                        break;
                    case WAITACK_5_CSMA:
                        updateStatusWaitAckCSMA(event, msg);
                        break;
                    case WAITSIFS_6_CSMA:
                        updateStatusSIFSCSMA(event, msg);
                        break;
                    case TRANSMITACK_7_CSMA:
                        updateStatusTransmitAckCSMA(event, msg);
                        break;
                    default:
                        EV << "Error in CSMA FSM: an unknown state has been reached. macState=" << macState << endl;
                        break;
                    }
                }
            }


            void csmaMain2::manageQueueCSMA() {
                if (macQueue.size() != 0) {
                    EV<< "(manageQueue) there are " << macQueue.size() << " packets to send, entering backoff wait state." << endl;
                    if( transmissionAttemptInterruptedByRx) {
                        // resume a transmission cycle which was interrupted by
                        // a frame reception during CCA check
                        transmissionAttemptInterruptedByRx = false;
                    } else {
                        // initialize counters if we start a new transmission
                        // cycle from zero
                        NB = 0;
                        //BE = macMinBE;
                    }
                    /*if(! backoffTimer->isScheduled()) {
                      startTimer(TIMER_BACKOFF);
                    }
                    updateMacState(BACKOFF_2);*/
                } else {
                    EV << "(manageQueue) no packets to send, entering IDLE state." << endl;
                    updateMacStateCSMA(IDLE_1_CSMA);
                    phy->setRadioState(MiximRadio::SLEEP);
                }
            }

            void csmaMain2::updateMacStateCSMA(t_mac_states_csma newMacState) {
                macStateCSMA = newMacState;
            }

            /*
             * Called by the FSM machine when an unknown transition is requested.
             */
            void csmaMain2::fsmErrorCSMA(t_mac_event_csma event, cMessage *msg) {
                EV<< "FSM Error ! In state " << macState << ", received unknown event:" << event << "." << endl;
                if (msg != NULL)
                delete msg;
            }

            void csmaMain2::startTimerCSMA(t_mac_timer_csma timer) {
                if (timer == TIMER_BACKOFF_CSMA) {
                    scheduleAt(scheduleBackoff(), backoffTimerCSMA);
                } else if (timer == TIMER_CCA_CSMA) {
                    simtime_t ccaTime = rxSetupTime + ccaDetectionTime;
                    debugEV<< "(startTimer) ccaTimer value=" << ccaTime
                    << "(rxSetupTime,ccaDetectionTime:" << rxSetupTime
                    << "," << ccaDetectionTime <<")." << endl;
                    scheduleAt(simTime()+rxSetupTime+ccaDetectionTime, ccaTimerCSMA);
                } else if (timer==TIMER_SIFS_CSMA) {
                    assert(useMACAcks);
                    debugEV << "(startTimer) sifsTimer value=" << sifs << endl;
                    scheduleAt(simTime()+sifs, sifsTimerCSMA);
                } else if (timer==TIMER_RX_ACK_CSMA) {
                    assert(useMACAcks);
                    debugEV << "(startTimer) rxAckTimer value=" << macAckWaitDuration << endl;
                    scheduleAt(simTime()+macAckWaitDuration, rxAckTimerCSMA);
                } else {
                    EV << "Unknown timer requested to start:" << timer << endl;
                }
            }


    simtime_t csmaMain2::scheduleBackoff() {

        simtime_t backoffTime;

        switch(backoffMethod) {
        case EXPONENTIAL:
        {
            int BE = std::min(macMinBE + NB, macMaxBE);
            int v = (1 << BE) - 1;
            int r = intuniform(0, v, 0);
            backoffTime = r * aUnitBackoffPeriod;

            debugEV<< "(startTimer) backoffTimer value=" << backoffTime
            << " (BE=" << BE << ", 2^BE-1= " << v << "r="
            << r << ")" << endl;
            break;
        }
        case LINEAR:
        {
            int slots = intuniform(1, initialCW + NB, 0);
            backoffTime = slots * aUnitBackoffPeriod;
            debugEV<< "(startTimer) backoffTimer value=" << backoffTime << endl;
            break;
        }
        case CONSTANT:
        {
            int slots = intuniform(1, initialCW, 0);
            backoffTime = slots * aUnitBackoffPeriod;
            debugEV<< "(startTimer) backoffTimer value=" << backoffTime << endl;
            break;
        }
        default:
            error("Unknown backoff method!");
            break;
        }

        nbBackoffs = nbBackoffs + 1;
        backoffValues = backoffValues + SIMTIME_DBL(backoffTime);

        return backoffTime + simTime();
    }

    /*
     * Binds timers to events and executes FSM.
     */
    void csmaMain2::handleSelfMsg(cMessage *msg) {
        debugEV<< "timer routine." << endl;
        if(msg==backoffTimer)
            executeMac(EV_TIMER_BACKOFF, msg);
        else if(msg==ccaTimer)
            executeMac(EV_TIMER_CCA, msg);
        else if(msg==sifsTimer)
            executeMac(EV_TIMER_SIFS, msg);
        else if(msg==rxAckTimer) {
            nbMissedAcks++;
            executeMac(EV_ACK_TIMEOUT, msg);
        }else if(msg==backoffTimerCSMA)
            executeMacCSMA(EV_TIMER_BACKOFF_CSMA, msg);
        else if(msg==ccaTimerCSMA)
            executeMacCSMA(EV_TIMER_CCA_CSMA, msg);
        else if(msg==sifsTimerCSMA)
            executeMacCSMA(EV_TIMER_SIFS_CSMA, msg);
        else if(msg==rxAckTimerCSMA) {
            nbMissedAcksCSMA++;
            executeMacCSMA(EV_ACK_TIMEOUT_CSMA, msg);
        } else
            EV << "CSMA Error: unknown timer fired:" << msg << endl;
    }

    /**
     * Compares the address of this Host with the destination address in
     * frame. Generates the corresponding event.
     */
    void csmaMain2::handleLowerMsg(cMessage *msg) {
        EV << "handle lower message avant le test...\n";
           EV<< "Received frame name= " << msg->getName() << endl;
           EV << "Received frame kind= " << msg->getKind() << endl;
        //if (msg->getKind() == APP_DATA_MESSAGE || msg->getKind() ==APP_DATA_MESSAGE_CSMA){
           if (msg->getKind() == DATA_MESSAGE){
            EV << "handle lower message dans le test...\n";

        macpkt_ptr_t            macPkt     = static_cast<macpkt_ptr_t> (msg);
        const LAddress::L2Type& src        = macPkt->getSrcAddr();
        const LAddress::L2Type& dest       = macPkt->getDestAddr();
        long                    ExpectedNr = 0;

        debugEV<< "Received frame name= " << macPkt->getName()
        << ", myState=" << macState << " src=" << src
        << " dst=" << dest << " myAddr="
        << myMacAddr << endl;

        if(dest == myMacAddr)
        {
            if(!useMACAcks) {
                debugEV << "Received a data packet addressed to me." << endl;
    //          nbRxFrames++;
                executeMac(EV_FRAME_RECEIVED, macPkt);
            }
            else {
                long SeqNr = macPkt->getSequenceId();

                if(strcmp(macPkt->getName(), "CSMA-Ack") != 0) {
                    // This is a data message addressed to us
                    // and we should send an ack.
                    // we build the ack packet here because we need to
                    // copy data from macPkt (src).
                    EV << "Received a data packet addressed to me,"
                       << " preparing an ack..." << endl;

    //              nbRxFrames++;

                    if(ackMessage != NULL)
                        delete ackMessage;
                    ackMessage = new MacPkt("CSMA-Ack", ACK);
                    ackMessage->setSrcAddr(myMacAddr);
                    ackMessage->setDestAddr(src);
                    ackMessage->setBitLength(ackLength);
                    //Check for duplicates by checking expected seqNr of sender
                    if(SeqNrChild.find(src) == SeqNrChild.end()) {
                        //no record of current child -> add expected next number to map
                        SeqNrChild[src] = SeqNr + 1;
                        EV << "Adding a new child to the map of Sequence numbers:" << src << endl;
                        executeMac(EV_FRAME_RECEIVED, macPkt);
                    }
                    else {
                        ExpectedNr = SeqNrChild[src];
                        EV << "Expected Sequence number is " << ExpectedNr <<
                        " and number of packet is " << SeqNr << endl;
                        if(SeqNr < ExpectedNr) {
                            //Duplicate Packet, count and do not send to upper layer
                            nbDuplicates++;
                            executeMac(EV_DUPLICATE_RECEIVED, macPkt);
                        }
                        else {
                            SeqNrChild[src] = SeqNr + 1;
                            executeMac(EV_FRAME_RECEIVED, macPkt);
                        }
                    }

                } else if(macQueue.size() != 0) {

                    // message is an ack, and it is for us.
                    // Is it from the right node ?
                    macpkt_ptr_t firstPacket = static_cast<macpkt_ptr_t>(macQueue.front());
                    if(src == firstPacket->getDestAddr()) {
                        nbRecvdAcks++;
                        executeMac(EV_ACK_RECEIVED, macPkt);
                    } else {
                        EV << "Error! Received an ack from an unexpected source: src=" << src << ", I was expecting from node addr=" << firstPacket->getDestAddr() << endl;
                        delete macPkt;
                    }
                } else {
                    EV << "Error! Received an Ack while my send queue was empty. src=" << src << "." << endl;
                    delete macPkt;
                }
            }
        }
        else if (LAddress::isL2Broadcast(dest)) {
            executeMac(EV_BROADCAST_RECEIVED, macPkt);
        } else {
            EV << "packet not for me, deleting...\n";
            delete macPkt;
        }
    }

        else if (msg->getKind() ==DATA_MESSAGE_CSMA){
                    EV << "handle lower message dans le test...\n";

                macpkt_ptr_t            macPkt     = static_cast<macpkt_ptr_t> (msg);
                const LAddress::L2Type& src        = macPkt->getSrcAddr();
                const LAddress::L2Type& dest       = macPkt->getDestAddr();
                long                    ExpectedNr = 0;

                debugEV<< "Received frame name= " << macPkt->getName()
                << ", myState=" << macState << " src=" << src
                << " dst=" << dest << " myAddr="
                << myMacAddr << endl;

                if(dest == myMacAddr)
                {
                    if(!useMACAcks) {
                        debugEV << "Received a data packet addressed to me." << endl;
            //          nbRxFrames++;
                        executeMacCSMA(EV_FRAME_RECEIVED_CSMA, macPkt);
                    }
                    else {
                        long SeqNr = macPkt->getSequenceId();

                        if(strcmp(macPkt->getName(), "CSMA-Ack") != 0) {
                            // This is a data message addressed to us
                            // and we should send an ack.
                            // we build the ack packet here because we need to
                            // copy data from macPkt (src).
                            EV << "Received a data packet addressed to me,"
                               << " preparing an ack..." << endl;

            //              nbRxFrames++;

                            if(ackMessage != NULL)
                                delete ackMessage;
                            ackMessage = new MacPkt("CSMA-Ack", ACK_CSMA);
                            ackMessage->setSrcAddr(myMacAddr);
                            ackMessage->setDestAddr(src);
                            ackMessage->setBitLength(ackLength);
                            //Check for duplicates by checking expected seqNr of sender
                            if(SeqNrChild.find(src) == SeqNrChild.end()) {
                                //no record of current child -> add expected next number to map
                                SeqNrChild[src] = SeqNr + 1;
                                EV << "Adding a new child to the map of Sequence numbers:" << src << endl;
                                executeMacCSMA(EV_FRAME_RECEIVED_CSMA, macPkt);
                            }
                            else {
                                ExpectedNr = SeqNrChild[src];
                                EV << "Expected Sequence number is " << ExpectedNr <<
                                " and number of packet is " << SeqNr << endl;
                                if(SeqNr < ExpectedNr) {
                                    //Duplicate Packet, count and do not send to upper layer
                                    nbDuplicates++;
                                    executeMacCSMA(EV_DUPLICATE_RECEIVED_CSMA, macPkt);
                                }
                                else {
                                    SeqNrChild[src] = SeqNr + 1;
                                    executeMacCSMA(EV_FRAME_RECEIVED_CSMA, macPkt);
                                }
                            }

                        } else if(macQueue.size() != 0) {

                            // message is an ack, and it is for us.
                            // Is it from the right node ?
                            macpkt_ptr_t firstPacket = static_cast<macpkt_ptr_t>(macQueue.front());
                            if(src == firstPacket->getDestAddr()) {
                                nbRecvdAcksCSMA++;
                                executeMacCSMA(EV_ACK_RECEIVED_CSMA, macPkt);
                            } else {
                                EV << "Error! Received an ack from an unexpected source: src=" << src << ", I was expecting from node addr=" << firstPacket->getDestAddr() << endl;
                                delete macPkt;
                            }
                        } else {
                            EV << "Error! Received an Ack while my send queue was empty. src=" << src << "." << endl;
                            delete macPkt;
                        }
                    }
                }
                else if (LAddress::isL2Broadcast(dest)) {
                    executeMacCSMA(EV_BROADCAST_RECEIVED_CSMA, macPkt);
                } else {
                    EV << "packet not for me, deleting...\n";
                    delete macPkt;
                }
            }

        else if (msg->getKind() == ACK){
            EV << "handle lower message dans le test...\n";
                MacPkt *macPkt = static_cast<MacPkt *> (msg);
                if(macQueue.size() != 0) {
               // message is an ack, and it is for us.
               // Is it from the right node ?
                    MacPkt * firstPacket = static_cast<MacPkt *>(macQueue.front());
                    if(macPkt->getSrcAddr() == firstPacket->getDestAddr()) {
                        nbRecvdAcks++;
                        executeMac(EV_ACK_RECEIVED, macPkt);
                    } else {
                        EV << "Error! Received an ack from an unexpected source: src=" << macPkt->getSrcAddr() << ", I was expecting from node addr=" << firstPacket->getDestAddr() << endl;
                        delete macPkt;
                    }
                } else {
                    EV << "Error! Received an Ack while my send queue was empty. src=" << macPkt->getSrcAddr() << "." << endl;
                    delete macPkt;
                }
            }
        else if (msg->getKind() == ACK_CSMA){
                    EV << "handle lower message dans le test...\n";
                        MacPkt *macPkt = static_cast<MacPkt *> (msg);
                        if(macQueue.size() != 0) {
                       // message is an ack, and it is for us.
                       // Is it from the right node ?
                            MacPkt * firstPacket = static_cast<MacPkt *>(macQueue.front());
                            if(macPkt->getSrcAddr() == firstPacket->getDestAddr()) {
                                nbRecvdAcksCSMA++;
                                executeMacCSMA(EV_ACK_RECEIVED_CSMA, macPkt);
                            } else {
                                EV << "Error! Received an ack from an unexpected source: src=" << macPkt->getSrcAddr() << ", I was expecting from node addr=" << firstPacket->getDestAddr() << endl;
                                delete macPkt;
                            }
                        } else {
                            EV << "Error! Received an Ack while my send queue was empty. src=" << macPkt->getSrcAddr() << "." << endl;
                            delete macPkt;
                        }
                    }
           else{
                   delete(msg);
               }
    }

    void csmaMain2::handleLowerControl(cMessage *msg) {
        if (msg->getKind() == MacToPhyInterface::TX_OVER) {
            executeMac(EV_FRAME_TRANSMITTED, msg);
            executeMacCSMA(EV_FRAME_TRANSMITTED_CSMA, msg);
        } else if (msg->getKind() == BaseDecider::PACKET_DROPPED) {
            EV<< "control message: PACKED DROPPED" << endl;
        } else if (msg->getKind() == MacToPhyInterface::RADIO_SWITCHING_OVER) {
            EV<< "control message: RADIO_SWITCHING_OVER" << endl;
        }  else if (msg->getKind() == IControl::AWAKE_PCA) {
            EV<< "control message: MAIN_RADIO_TURNED_ON" << endl;
           // phy->setRadioState(MiximRadio::RX);
            sendControlUp(msg);
            EV<< " MAIN_RADIO_TURNED_ON sent to up " << endl;
        }else if (msg->getKind() == IControl::AWAKE_CSMA) {
                        EV<< "control message: MAIN_RADIO_TURNED_ON" << endl;
                        //phy->setRadioState(MiximRadio::RX);
                        sendControlUp(msg);
                        EV<< " MAIN_RADIO_TURNED_ON sent to up " << endl;
        }else if (msg->getKind() == IControl::SLEEPING_PCA) {
            EV<< "control message: MAIN_RADIO_TURNED_OFF" << endl;
            //phy->setRadioState(Radio::SLEEP);
            sendControlUp(msg);
            EV<< " MAIN_RADIO_TURNED_ON sent to up " << endl;
        }else if (msg->getKind() == IControl::SLEEPING_CSMA) {
                    EV<< "control message: MAIN_RADIO_TURNED_OFF" << endl;
                    //phy->setRadioState(Radio::SLEEP);
                    sendControlUp(msg);
                    EV<< " MAIN_RADIO_TURNED_ON sent to up " << endl;
        } else if (msg->getKind() == MacToPhyInterface::RADIO_SWITCHING_OVER) {
            EV<< "control message: RADIO_SWITCHING_OVER" << endl;
            //nouveau code
            //sendControlUp(msg);
        } else if (msg->getKind() == Decider802154Narrow::RECEPTION_STARTED) {
            debugEV<< "control message: RECEIVING AirFrame" << endl;
            delete msg;
            return;
        } else {
            EV << "Invalid control message type (type=NOTHING) : name="
            << msg->getName() << " modulesrc="
            << msg->getSenderModule()->getFullPath()
            << "." << endl;
        }
        //delete msg;
    }

    cPacket *csmaMain2::decapsMsg(macpkt_ptr_t macPkt) {
        cPacket * msg = macPkt->decapsulate();
        setUpControlInfo(msg, macPkt->getSrcAddr());

        return msg;
    }

    void csmaMain2::handleUpperControl(cMessage *msg) {
        if (msg->getKind() == IControl::POWER_ON_PCA) {
            EV<< "control message: POWER_ON_PCA" << endl;
            //phy->setRadioState(Radio::RX);
            sendControlDown(msg);
            EV<< " POWER_ON_PCA sent to down " << endl;
        }else if (msg->getKind() == IControl::POWER_OFF_PCA) {
            EV<< "control message: POWER_OFF_PCA" << endl;
            //phy->setRadioState(Radio::SLEEP);

            sendControlDown(msg);
            EV<< " TURN_OFF sent to down " << endl;
        } else if (msg->getKind() == IControl::POWER_ON_CSMA) {
            EV<< "control message: POWER_ON_CSMA" << endl;
            //phy->setRadioState(Radio::RX);
            sendControlDown(msg);
            EV<< " TURN_ON_CSMA sent to down " << endl;
        }else if (msg->getKind() == IControl::POWER_OFF_CSMA) {
            EV<< "control message: POWER_OFF_CSMA" << endl;
            //phy->setRadioState(Radio::SLEEP);

            sendControlDown(msg);
            EV<< " POWER_OFF_CSMA sent to down " << endl;
        }else {
                sendControlDown(msg);
                EV << "Control Msg sent down";
            }

    }
