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

#include "D2MACWuR.h"
#include <cassert>
#include <map>
#include "FWMath.h"
#include "BaseDecider.h"
#include "BaseArp.h"
#include "BasePhyLayer.h"
#include "BaseConnectionManager.h"
#include "FindModule.h"
#include "MacPkt_m.h"
#include "Decider802154Narrow.h"

Define_Module(D2MACWuR);



void D2MACWuR::initialize(int stage)
{
    BaseMacLayer::initialize(stage);

        if (stage == 0) {
            BaseLayer::catDroppedPacketSignal.initialize();

            useMACAcks = par("useMACAcks").boolValue();
            queueLength = par("queueLength");
            sifs = par("sifs");
            transmissionAttemptInterruptedByRx = false;
            nbTxWuC = 0;
            nbTxWuCCSMA = 0;
            nbRxWuC = 0;
            nbRxWuCCSMA = 0;
            nbMissedAcks = 0;
            nbTxAcks = 0;
            nbRecvdAcks = 0;
            nbDroppedWuC = 0;
            nbDroppedWuCCSMA=0;
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
            bitrate = par("bitrate");
            ackLength = par("ackLength");
            macCritDelay=par("macCritDelay").doubleValue();;
            ackMessage = NULL;
            delayBackoff=0;

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
            //delay=0.00586;  //0
            delay=0;  //0
            elapsedTime=0;

            txPower = par("txPower").doubleValue();

            droppedPacket.setReason(DroppedPacket::NONE);
            nicId = getNic()->getId();

            // initialize the timers
            backoffTimer = new cMessage("timer-backoff");
            ccaTimer = new cMessage("timer-cca");
            sifsTimer = new cMessage("timer-sifs");
            rxAckTimer = new cMessage("timer-rxAck");
            backoffTimerPCA = new cMessage("timer-backoff-pca");
            ccaTimerPCA = new cMessage("timer-cca-pca");
            sifsTimerPCA = new cMessage("timer-sifs-pca");
            rxAckTimerPCA = new cMessage("timer-rxAck-pca");
            delaybackoffTimerPCA = new cMessage("delaybackoffTimerPCA");
            macState = IDLE_1;
            macStatePCA = IDLE_1_PCA;
            txAttempts = 0;

            //firstPacketGeneration = -1;
            //lastPacketReception = -2;



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

void D2MACWuR::finish() {
    if (stats) {
        //recordScalar("nbTxFrames", nbTxFrames);
       // recordScalar("nbRxFrames", nbRxFrames);
       // recordScalar("nbDroppedFrames", nbDroppedFrames);
       // recordScalar("sojournTime", simTime() - arrivalTime, "s");
        recordScalar("nbTxWuC", nbTxWuC);
        recordScalar("nbTxWuCCSMA", nbTxWuCCSMA);
        recordScalar("nbRxWuC", nbRxWuC);
        recordScalar("nbRxWuCCSMA", nbRxWuCCSMA);
        recordScalar("nbDroppedWuC", nbDroppedWuC);
        recordScalar("nbDroppedWuCCCSMA", nbDroppedWuCCSMA);
        recordScalar("nbMissedAcks", nbMissedAcks);
        recordScalar("nbRecvdAcks", nbRecvdAcks);
        recordScalar("nbTxAcks", nbTxAcks);
        recordScalar("nbDuplicates", nbDuplicates);
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

D2MACWuR::~D2MACWuR() {
    cancelAndDelete(backoffTimer);
    cancelAndDelete(ccaTimer);
    cancelAndDelete(sifsTimer);
    cancelAndDelete(rxAckTimer);
    cancelAndDelete(backoffTimerPCA);
        cancelAndDelete(ccaTimerPCA);
        cancelAndDelete(sifsTimerPCA);
        cancelAndDelete(rxAckTimerPCA);
        cancelAndDelete(delaybackoffTimerPCA);
    if (ackMessage)
        delete ackMessage;
    MacQueue::iterator it;
    for (it = macQueue.begin(); it != macQueue.end(); ++it) {
        delete (*it);
    }
}

cStdDev& D2MACWuR::hostsLatency(const LAddress::L2Type& hostAddress)
{
    using std::pair;

    if(latencies.count(hostAddress) == 0) {
        std::ostringstream oss;
        oss << hostAddress;
        cStdDev aLatency(oss.str().c_str());
        latencies.insert(pair<LAddress::L2Type, cStdDev>(hostAddress, aLatency));
    }

    return latencies[hostAddress];
}


/**
 * Encapsulates the message to be transmitted and pass it on
 * to the FSM main method for further processing.
 */
void D2MACWuR::handleUpperMsg(cMessage *msg) {
    //macpkt_ptr_tmacPkt = encapsMsg(msg);
    EV<< "Received frame WUC_MESSAGE before testing (from upper layer) " << endl;
   EV<< "Received frame name= " << msg->getName() << endl;
    EV << "Message Name=" << msg->getName() << " and Message Type= " <<  msg->getKind() << "\n";
   // simtime_t theLatency;
    arrivalTime=msg->getArrivalTime();
    EV<< "the Latency: getArrivalTime()" << msg->getArrivalTime()

                                         << ", the Latency: getSendingTime()="<< msg->getSendingTime()
                                         << ", the Latency: getTimestamp()="<< msg->getTimestamp()
                                << ", the Latency: getCreationTime()=" << msg->getCreationTime() << endl;
    theLatency = msg->getArrivalTime() - msg->getCreationTime();
    EV<<  "latency=" << theLatency << endl;
   // delay= theLatency;

    if (msg->getKind() == WUC_MESSAGE_PCA){
                EV<< "starting test (WUC_MESSAGE_PCA)..."<< endl;
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
                    executeMacPCA(EV_SEND_REQUEST_PCA, macPkt);
        }


    else if (msg->getKind() == WUC_MESSAGE_CSMA){
        EV<< "starting test..."<< endl;
    macpkt_ptr_t macPkt = new MacPkt(msg->getName(), msg->getKind());
    macPkt->setBitLength(headerLength);
    cObject *const cInfo = msg->removeControlInfo();
    debugEV<<"CSMA received a message from upper layer, name is " << msg->getName() <<", CInfo removed, mac addr="<< getUpperDestinationFromControlInfo(cInfo) << endl;
    LAddress::L2Type dest = getUpperDestinationFromControlInfo(cInfo);
    macPkt->setDestAddr(dest);
    delete cInfo;
    macPkt->setSrcAddr(myMacAddr);

    if(useMACAcks) {
        if(SeqNrParent.find(dest) == SeqNrParent.end()) {
            //no record of current parent -> add next sequence number to map
            SeqNrParent[dest] = 1;
            macPkt->setSequenceId(0);
            debugEV << "Adding a new parent to the map of Sequence numbers:" << dest << endl;
        }
        else {
            macPkt->setSequenceId(SeqNrParent[dest]);
            debugEV << "Packet send with sequence number = " << SeqNrParent[dest] << endl;
            SeqNrParent[dest]++;
        }
    }

    //RadioAccNoise3PhyControlInfo *pco = new RadioAccNoise3PhyControlInfo(bitrate);
    //macPkt->setControlInfo(pco);
    assert(static_cast<cPacket*>(msg));
    macPkt->encapsulate(static_cast<cPacket*>(msg));
    debugEV <<"pkt encapsulated, length: " << macPkt->getBitLength() << "\n";
    executeMac(EV_SEND_REQUEST, macPkt);
}
    else{
        EV <<"delete message " <<  "\n";
            delete(msg);
        }
}

void D2MACWuR::updateStatusIdle(t_mac_event event, cMessage *msg) {
    switch (event) {
    case EV_SEND_REQUEST:
        if (macQueue.size() <= queueLength) {
            macQueue.push_back(static_cast<macpkt_ptr_t> (msg));
            EV<<"(1) FSM State IDLE_1, EV_SEND_REQUEST and [TxBuff avail]: startTimerBackOff -> BACKOFF." << endl;
            updateMacState(BACKOFF_2);
            NB = 0;
            //comment
            BE = macMinBE;
            startTimer(TIMER_BACKOFF);
        } else {
            // queue is full, message has to be deleted
            EV << "(12) FSM State IDLE_1, EV_SEND_REQUEST and [TxBuff not avail]: dropping packet -> IDLE." << endl;
            msg->setName("MAC ERROR");
            msg->setKind(PACKET_DROPPED);
            sendControlUp(msg);
            droppedPacket.setReason(DroppedPacket::QUEUE);
            emit(BaseLayer::catDroppedPacketSignal, &droppedPacket);
            updateMacState(IDLE_1);
        }
        break;
    case EV_DUPLICATE_RECEIVED:
        debugEV << "(15) FSM State IDLE_1, EV_DUPLICATE_RECEIVED: setting up radio tx -> WAITSIFS." << endl;
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
        nbRxWuCCSMA++;
        delete msg;

        if(useMACAcks) {
            phy->setRadioState(MiximRadio::TX);
            updateMacState(WAITSIFS_6);
            startTimer(TIMER_SIFS);
        }
        break;

    case EV_BROADCAST_RECEIVED:
        debugEV << "(23) FSM State IDLE_1, EV_BROADCAST_RECEIVED: Nothing to do." << endl;
        nbRxWuCCSMA++;
        sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
        delete msg;
        break;
    default:
        fsmError(event, msg);
        break;
    }
}

void D2MACWuR::updateStatusBackoff(t_mac_event event, cMessage *msg) {
    switch (event) {
    case EV_TIMER_BACKOFF:
        EV<< "(2) FSM State BACKOFF, EV_TIMER_BACKOFF:"
        << " starting CCA timer." << endl;
        EV<< "(2) FSM State BACKOFF, EV_TIMER_BACKOFF:" << endl;
        startTimer(TIMER_CCA);
        updateMacState(CCA_3);
        phy->setRadioState(MiximRadio::RX);
        break;
    case EV_DUPLICATE_RECEIVED:
        // suspend current transmission attempt,
        // transmit ack,
        // and resume transmission when entering manageQueue()
        debugEV << "(28) FSM State BACKOFF, EV_DUPLICATE_RECEIVED:";
        if(useMACAcks) {
            debugEV << "suspending current transmit tentative and transmitting ack";
            transmissionAttemptInterruptedByRx = true;
            cancelEvent(backoffTimer);
            phy->setRadioState(MiximRadio::TX);
            updateMacState(WAITSIFS_6);
            startTimer(TIMER_SIFS);
        } else {
            debugEV << "Nothing to do.";
        }
        //sendUp(decapsMsg(static_cast<MacSeqPkt *>(msg)));
        delete msg;

        break;
    case EV_FRAME_RECEIVED:
        // suspend current transmission attempt,
        // transmit ack,
        // and resume transmission when entering manageQueue()
        debugEV << "(28) FSM State BACKOFF, EV_FRAME_RECEIVED:";
        if(useMACAcks) {
            debugEV << "suspending current transmit tentative and transmitting ack";
            transmissionAttemptInterruptedByRx = true;
            cancelEvent(backoffTimer);

            phy->setRadioState(MiximRadio::TX);
            updateMacState(WAITSIFS_6);
            startTimer(TIMER_SIFS);
        } else {
            debugEV << "sending frame up and resuming normal operation.";
        }
        sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
        delete msg;
        break;
    case EV_BROADCAST_RECEIVED:
        debugEV << "(29) FSM State BACKOFF, EV_BROADCAST_RECEIVED:"
        << "sending frame up and resuming normal operation." <<endl;
        sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
        delete msg;
        break;
    default:
        fsmError(event, msg);
        break;
    }
}

void D2MACWuR::attachSignal(macpkt_ptr_t mac, simtime_t_cref startTime) {
    simtime_t duration = (mac->getBitLength() + phyHeaderLength)/bitrate;
    setDownControlInfo(mac, createSignal(startTime, duration, txPower, bitrate));
}

void D2MACWuR::updateStatusCCA(t_mac_event event, cMessage *msg) {
    switch (event) {
    case EV_TIMER_CCA:
    {
        EV<< "(25) FSM State CCA_3, EV_TIMER_CCA" << endl;
        bool isIdle = phy->getChannelState().isIdle();
        if(isIdle) {
            EV << "(3) FSM State CCA_3, EV_TIMER_CCA, [Channel Idle]: -> TRANSMITFRAME_4." << endl;
            updateMacState(TRANSMITFRAME_4);
            phy->setRadioState(MiximRadio::TX);
            macpkt_ptr_t mac = check_and_cast<macpkt_ptr_t>(macQueue.front()->dup());

           //mac = static_cast<cMessage *>(macQueue.front());
                   // macQueue.pop_front();

            attachSignal(mac, simTime()+aTurnaroundTime);
            //sendDown(msg);
            // give time for the radio to be in Tx state before transmitting
            sendDelayed(mac, aTurnaroundTime, lowerLayerOut);
            EV << "Message Name=" << mac->getName() << " and Message Type= " <<  mac->getKind() << "\n";
            EV << "WuC CSMA sent to phy layer " << endl;
            nbTxWuCCSMA++;
            //Nouveau code
            //cMessage * m=macQueue.front();
           // cMessage * m = static_cast<cMessage *>(macQueue.front()->dup());
                 //   macQueue.pop_front();
           // cMessage * m = macQueue.front();
                            //macQueue.pop_front();
            cMessage * m = new cMessage;
            m->setName("WUC_START_TX_CSMA");
            m->setKind(WUC_START_TX_CSMA);
            sendControlUp(m);
           // send(m, upperControlOut);
            EV << "WUC_START_TX sent to up layers " << endl;

        } else {
            // Channel was busy, increment 802.15.4 backoff timers as specified.
            EV << "(7) FSM State CCA_3, EV_TIMER_CCA, [Channel Busy]: "
            << " increment counters." << endl;
            NB = NB+1;
            //comment
            BE = std::min(BE+1, macMaxBE);

            EV << "NB counter=" << NB << endl;
            // decide if we go for another backoff or if we drop the frame.
            if(NB> macMaxCSMABackoffs) {
                // drop the frame
                EV << "Tried " << NB << " backoffs, all reported a busy "
                << "channel. Dropping the packet." << endl;
                cMessage * mac = macQueue.front();
                macQueue.pop_front();
                txAttempts = 0;
                nbDroppedWuCCSMA++;
                mac->setName("MAC ERROR");
                mac->setKind(PACKET_DROPPED);
                sendControlUp(mac);
                manageQueue();
            } else {
                // redo backoff
                EV << "redo backoff"<< endl;
                updateMacState(BACKOFF_2);
                startTimer(TIMER_BACKOFF);
            }
        }
        break;
    }
    case EV_DUPLICATE_RECEIVED:
        EV << "(26) FSM State CCA_3, EV_DUPLICATE_RECEIVED:";
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
            EV << " Nothing to do." << endl;
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
            EV << " Nothing to do." << endl;
        }
        sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
        delete msg;
        break;
    case EV_BROADCAST_RECEIVED:
        EV << "(24) FSM State BACKOFF, EV_BROADCAST_RECEIVED:"
        << " Nothing to do." << endl;
        sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
        delete msg;
        break;
    default:
        fsmError(event, msg);
        break;
    }
}

void D2MACWuR::updateStatusTransmitFrame(t_mac_event event, cMessage *msg) {
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

void D2MACWuR::updateStatusWaitAck(t_mac_event event, cMessage *msg) {
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
        mac->setName("MAC SUCCESS");
        mac->setKind(TX_OVER);
        sendControlUp(mac);
        delete msg;
        manageQueue();
        break;
    case EV_ACK_TIMEOUT:
        EV << "(12) FSM State WAITACK_5, EV_ACK_TIMEOUT:"
        << " incrementCounter/dropPacket, manageQueue..." << endl;
        manageMissingAck(event, msg);
        break;
    case EV_BROADCAST_RECEIVED:
    case EV_FRAME_RECEIVED:
        sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
        break;
    case EV_DUPLICATE_RECEIVED:
        EV << "Error ! Received a frame during SIFS !" << endl;
        delete msg;
        break;
    default:
        fsmError(event, msg);
        break;
    }

}

void D2MACWuR::manageMissingAck(t_mac_event /*event*/, cMessage */*msg*/) {
    if (txAttempts < macMaxFrameRetries + 1) {
        // increment counter
        txAttempts++;
        EV<< "I will retransmit this packet (I already tried "
        << txAttempts << " times)." << endl;
    } else {
        // drop packet
        EV << "Packet was transmitted " << txAttempts
        << " times and I never got an Ack. I drop the packet." << endl;
        cMessage * mac = macQueue.front();
        macQueue.pop_front();
        txAttempts = 0;
        mac->setName("MAC ERROR");
        mac->setKind(PACKET_DROPPED);
        sendControlUp(mac);
    }
    manageQueue();
}
void D2MACWuR::updateStatusSIFS(t_mac_event event, cMessage *msg) {
    assert(useMACAcks);

    switch (event) {
    case EV_TIMER_SIFS:
        EV<< "(17) FSM State WAITSIFS_6, EV_TIMER_SIFS:"
        << " sendAck -> TRANSMITACK." << endl;
        updateMacState(TRANSMITACK_7);
        attachSignal(ackMessage, simTime());
        sendDown(ackMessage);
        nbTxAcks++;
        //      sendDelayed(ackMessage, aTurnaroundTime, lowerLayerOut);
        ackMessage = NULL;
        break;
    case EV_TIMER_BACKOFF:
        // Backoff timer has expired while receiving a frame. Restart it
        // and stay here.
        EV << "(16) FSM State WAITSIFS_6, EV_TIMER_BACKOFF. "
        << "Restart backoff timer and don't move." << endl;
        startTimer(TIMER_BACKOFF);
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

void D2MACWuR::updateStatusTransmitAck(t_mac_event event, cMessage *msg) {
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

void D2MACWuR::updateStatusNotIdle(cMessage *msg) {
    EV<< "(20) FSM State NOT IDLE, EV_SEND_REQUEST. Is a TxBuffer available ?" << endl;
    if (macQueue.size() <= queueLength) {
        macQueue.push_back(static_cast<macpkt_ptr_t>(msg));
        EV << "(21) FSM State NOT IDLE, EV_SEND_REQUEST"
        <<" and [TxBuff avail]: enqueue packet and don't move." << endl;
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
    }

}
/**
 * Updates state machine.
 */
void D2MACWuR::executeMac(t_mac_event event, cMessage *msg) {
    EV<< "In executeMac" << endl;
    if(macState != IDLE_1 && event == EV_SEND_REQUEST) {
        updateStatusNotIdle(msg);
    } else {
        switch(macState) {
        case IDLE_1:
            updateStatusIdle(event, msg);
            break;
        case BACKOFF_2:
            updateStatusBackoff(event, msg);
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

void D2MACWuR::manageQueue() {
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
            //comment
            BE = macMinBE;
        }
        if(! backoffTimer->isScheduled()) {
          startTimer(TIMER_BACKOFF);
        }
        updateMacState(BACKOFF_2);
    } else {
        EV << "(manageQueue) no packets to send, entering IDLE state." << endl;
        updateMacState(IDLE_1);

    }
}

void D2MACWuR::updateMacState(t_mac_states newMacState) {
    macState = newMacState;
}

/*
 * Called by the FSM machine when an unknown transition is requested.
 */
void D2MACWuR::fsmError(t_mac_event event, cMessage *msg) {
    EV<< "FSM Error ! In state " << macState << ", received unknown event:" << event << "." << endl;
    if (msg != NULL)
    delete msg;
}

void D2MACWuR::startTimer(t_mac_timer timer) {
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

simtime_t D2MACWuR::scheduleBackoff() {

    simtime_t backoffTime;

    switch(backoffMethod) {
    case EXPONENTIAL:
    {
        int BE = std::min(macMinBE + NB, macMaxBE);
        int v = (1 << BE) - 1;
        int r = intuniform(0, v, 0);
        //double r = intrand(v) + 1.0 + dblrand();
       // double slots = intrand(initialCW + txAttempts) + 1.0 + dblrand();
        backoffTime = r * aUnitBackoffPeriod;

        EV<< "(startTimer) backoffTimer value=" << backoffTime
        << " (BE=" << BE << ", 2^BE-1= " << v << " r="
        << r << ")" << endl;
        EV<< "backoffTime + simTime()= " << backoffTime + simTime() << endl;
            EV<< "nbBackoffs" << nbBackoffs << endl;
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
void D2MACWuR::handleSelfMsg(cMessage *msg) {
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
    } else if(msg==backoffTimerPCA)
    executeMacPCA(EV_TIMER_BACKOFF_PCA, msg);
    else if(msg==ccaTimerPCA)
        executeMacPCA(EV_TIMER_CCA_PCA, msg);
    else if(msg==sifsTimerPCA)
        executeMacPCA(EV_TIMER_SIFS_PCA, msg);
    else if(msg==rxAckTimerPCA) {
        nbMissedAcks++;
        executeMacPCA(EV_ACK_TIMEOUT_PCA, msg);
    }
    else
        EV << "CSMA Error: unknown timer fired:" << msg << endl;
}

/**
 * Compares the address of this Host with the destination address in
 * frame. Generates the corresponding event.
 */
void D2MACWuR::handleLowerMsg(cMessage *msg) {
     EV << "handle lower message avant le test...\n";
        EV<< "Received frame name= " << msg->getName() << endl;
        EV << "Received frame kind= " << msg->getKind() << endl;
    //if (msg->getKind() == BaseMacLayer::APP_WUC_MESSAGE){
    if (msg->getKind() == WUC_MESSAGE_CSMA){
            EV << "handle lower message dans le test...\n";

    macpkt_ptr_t            macPkt     = static_cast<macpkt_ptr_t> (msg);
    const LAddress::L2Type& src        = macPkt->getSrcAddr();
    const LAddress::L2Type& dest       = macPkt->getDestAddr();
    long                    ExpectedNr = 0;

    EV<< "Received frame name= " << macPkt->getName()
    << ", myState=" << macState << " src=" << src
    << " dst=" << dest << " myAddr="
    << myMacAddr << endl;

    if(dest == myMacAddr)
    {
        if(!useMACAcks) {
            EV << "Received a data packet addressed to me." << endl;
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
                debugEV << "Received a data packet addressed to me,"
                   << " preparing an ack..." << endl;

//              nbRxFrames++;

                if(ackMessage != NULL)
                    delete ackMessage;
                ackMessage = new MacPkt("CSMA-Ack");
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

    else if (msg->getKind() ==WUC_MESSAGE_PCA){
                EV<< "starting test (APP_WUC_MESSAGE_PCA)..."<< endl;

                macpkt_ptr_t            macPkt     = static_cast<macpkt_ptr_t> (msg);
                    const LAddress::L2Type& src        = macPkt->getSrcAddr();
                    const LAddress::L2Type& dest       = macPkt->getDestAddr();
                    long                    ExpectedNr = 0;

                    EV<< "Received frame name= " << macPkt->getName()
                    << ", myState=" << macState << " src=" << src
                    << " dst=" << dest << " myAddr="
                    << myMacAddr << endl;

                    if(dest == myMacAddr)
                    {
                        if(!useMACAcks) {
                            EV << "Received a data packet addressed to me." << endl;
                //          nbRxFrames++;
                            executeMacPCA(EV_FRAME_RECEIVED_PCA, macPkt);
                        }
                        else {
                            long SeqNr = macPkt->getSequenceId();

                            if(strcmp(macPkt->getName(), "CSMA-Ack") != 0) {
                                // This is a data message addressed to us
                                // and we should send an ack.
                                // we build the ack packet here because we need to
                                // copy data from macPkt (src).
                                debugEV << "Received a data packet addressed to me,"
                                   << " preparing an ack..." << endl;

                //              nbRxFrames++;

                                if(ackMessage != NULL)
                                    delete ackMessage;
                                ackMessage = new MacPkt("CSMA-Ack");
                                ackMessage->setSrcAddr(myMacAddr);
                                ackMessage->setDestAddr(src);
                                ackMessage->setBitLength(ackLength);
                                //Check for duplicates by checking expected seqNr of sender
                                if(SeqNrChild.find(src) == SeqNrChild.end()) {
                                    //no record of current child -> add expected next number to map
                                    SeqNrChild[src] = SeqNr + 1;
                                    EV << "Adding a new child to the map of Sequence numbers:" << src << endl;
                                    executeMacPCA(EV_FRAME_RECEIVED_PCA, macPkt);
                                }
                                else {
                                    ExpectedNr = SeqNrChild[src];
                                    EV << "Expected Sequence number is " << ExpectedNr <<
                                    " and number of packet is " << SeqNr << endl;
                                    if(SeqNr < ExpectedNr) {
                                        //Duplicate Packet, count and do not send to upper layer
                                        nbDuplicates++;
                                        executeMacPCA(EV_DUPLICATE_RECEIVED_PCA, macPkt);
                                    }
                                    else {
                                        SeqNrChild[src] = SeqNr + 1;
                                        executeMacPCA(EV_FRAME_RECEIVED_PCA, macPkt);
                                    }
                                }

                            } else if(macQueue.size() != 0) {

                                // message is an ack, and it is for us.
                                // Is it from the right node ?
                                macpkt_ptr_t firstPacket = static_cast<macpkt_ptr_t>(macQueue.front());
                                if(src == firstPacket->getDestAddr()) {
                                    nbRecvdAcks++;
                                    executeMacPCA(EV_ACK_RECEIVED_PCA, macPkt);
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
                        executeMacPCA(EV_BROADCAST_RECEIVED_PCA, macPkt);
                    } else {
                        EV << "packet not for me, deleting...\n";
                        delete macPkt;
                    }
            }
        else {
                delete (msg);
            }
}

void D2MACWuR::handleLowerControl(cMessage *msg) {
    if (msg->getKind() == MacToPhyInterface::TX_OVER) {
        executeMac(EV_FRAME_TRANSMITTED, msg);
        executeMacPCA(EV_FRAME_TRANSMITTED_PCA, msg);
    } else if (msg->getKind() == BaseDecider::PACKET_DROPPED) {
        debugEV<< "control message: PACKED DROPPED" << endl;
    } else if (msg->getKind() == MacToPhyInterface::RADIO_SWITCHING_OVER) {
        debugEV<< "control message: RADIO_SWITCHING_OVER" << endl;
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
    delete msg;
}

cPacket *D2MACWuR::decapsMsg(macpkt_ptr_t macPkt) {
    cPacket * msg = macPkt->decapsulate();
    setUpControlInfo(msg, macPkt->getSrcAddr());

    return msg;
}


//*****************************PCA*******************************************************************

void D2MACWuR::updateStatusIdlePCA(t_mac_event_pca event, cMessage *msg) {
    EV << " updateStatusIdlePCA(PCA): t_mac_event_PCA. event=" << event << endl;
    EV << " macStatePCA=" << macStatePCA << endl;
    switch (event) {
    case EV_SEND_REQUEST_PCA:
        if (macQueue.size() <= queueLength) {
            macQueue.push_back(static_cast<macpkt_ptr_t> (msg));
            EV<<"(1) FSM State IDLE_1, EV_SEND_REQUEST and [TxBuff avail]: startTimerBackOff -> BACKOFF." << endl;
            updateMacStatePCA(BACKOFF_2_PCA);
            //NB = 0;
            //BE = macMinBE;
            int BE = std::max(1, macMinBE - 1);
            int r = (1 << BE) - 1;
             TB = intuniform(0, r, 0);
            backoffTimePCA = TB * aUnitBackoffPeriod;
            //modif
            //delay=0.00568;
            //delay=theLatency+0.001568+0.000544+0.003;
         //   delay=theLatency;
            //*******************
            //int BE = std::min(macMinBE + NB, macMaxBE);
            // int v = (1 << BE) - 1;
             //int r = intuniform(0, v, 0);
             EV<< "(startTimer) backoffTimer value=" << backoffTimePCA
                    << " (BE=" << BE << ", 2^BE-1= " << r << ", TB="
                    << TB << ")" << endl;


            //scheduleAt(simTime()+backoffTimePCA, backoffTimerPCA);
             scheduleAt(simTime(), backoffTimerPCA);
            //startTimer(TIMER_BACKOFF);
        } else {
            // queue is full, message has to be deleted
            EV << "(12) FSM State IDLE_1, EV_SEND_REQUEST and [TxBuff not avail]: dropping packet -> IDLE." << endl;
            msg->setName("MAC ERROR");
            msg->setKind(PACKET_DROPPED);
            sendControlUp(msg);
            droppedPacket.setReason(DroppedPacket::QUEUE);
            emit(BaseLayer::catDroppedPacketSignal, &droppedPacket);
            updateMacStatePCA(IDLE_1_PCA);
        }
        break;
    case EV_DUPLICATE_RECEIVED_PCA:
        debugEV << "(15) FSM State IDLE_1, EV_DUPLICATE_RECEIVED: setting up radio tx -> WAITSIFS." << endl;
        //sendUp(decapsMsg(static_cast<MacSeqPkt *>(msg)));
        delete msg;

        if(useMACAcks) {
            phy->setRadioState(MiximRadio::TX);
            updateMacStatePCA(WAITSIFS_6_PCA);
            startTimerPCA(TIMER_SIFS_PCA);
        }
        break;

    case EV_FRAME_RECEIVED_PCA:
        EV << "(15) FSM State IDLE_1, EV_FRAME_RECEIVED: setting up radio tx -> WAITSIFS." << endl;
        sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
        nbRxWuC++;
        delete msg;

        if(useMACAcks) {
            phy->setRadioState(MiximRadio::TX);
            updateMacStatePCA(WAITSIFS_6_PCA);
            startTimerPCA(TIMER_SIFS_PCA);
        }
        break;

    case EV_BROADCAST_RECEIVED_PCA:
        debugEV << "(23) FSM State IDLE_1, EV_BROADCAST_RECEIVED: Nothing to do." << endl;
        nbRxWuC++;
        sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
        delete msg;
        break;
    default:
        fsmErrorPCA(event, msg);
        break;
    }
}

void D2MACWuR::updateStatusBackoffPCA(t_mac_event_pca event, cMessage *msg) {
    simtime_t theLatency;
    //macpkt_ptr_t            m ;
    switch (event) {
    case EV_TIMER_BACKOFF_PCA:

       // delay=delay+backoffTimePCA+delayBackoff;
        //delay=delay+delayBackoff;
        //delay=theLatency;
        EV <<" StatusBackoffPCA: Delay =" << delay << endl;
        EV <<" macCritDelay = " << macCritDelay << endl;

        if (delay> macCritDelay){

           // drop the frame
         EV <<" Dropping the packet." << endl;
         cMessage * mac = macQueue.front();
         macQueue.pop_front();
         txAttempts = 0;
         nbDroppedWuC++;
         mac->setName("MAC ERROR");
         mac->setKind(PACKET_DROPPED);
         sendControlUp(mac);
         manageQueuePCA();
                    }
         else {
         EV<< "(2) FSM State BACKOFF PCA, EV_TIMER_BACKOFF:"
         << " starting CCA timer." << endl;
         startTimerPCA(TIMER_CCA_PCA);
         updateMacStatePCA(CCA_3_PCA);
         phy->setRadioState(MiximRadio::RX);
                    }
                    break;
    case EV_DUPLICATE_RECEIVED_PCA:
        // suspend current transmission attempt,
        // transmit ack,
        // and resume transmission when entering manageQueue()
        debugEV << "(28) FSM State BACKOFF, EV_DUPLICATE_RECEIVED:";
        if(useMACAcks) {
            debugEV << "suspending current transmit tentative and transmitting ack";
            transmissionAttemptInterruptedByRx = true;
            cancelEvent(backoffTimer);
            phy->setRadioState(MiximRadio::TX);
            updateMacStatePCA(WAITSIFS_6_PCA);
            startTimerPCA(TIMER_SIFS_PCA);
        } else {
            debugEV << "Nothing to do.";
        }
        //sendUp(decapsMsg(static_cast<MacSeqPkt *>(msg)));
        delete msg;

        break;
    case EV_FRAME_RECEIVED_PCA:
        // suspend current transmission attempt,
        // transmit ack,
        // and resume transmission when entering manageQueue()
        debugEV << "(28) FSM State BACKOFF, EV_FRAME_RECEIVED:";
        if(useMACAcks) {
            debugEV << "suspending current transmit tentative and transmitting ack";
            transmissionAttemptInterruptedByRx = true;
            cancelEvent(backoffTimer);

            phy->setRadioState(MiximRadio::TX);
            updateMacStatePCA(WAITSIFS_6_PCA);
            startTimerPCA(TIMER_SIFS_PCA);
        } else {
            debugEV << "sending frame up and resuming normal operation.";
        }
        sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
        delete msg;
        break;
    case EV_BROADCAST_RECEIVED_PCA:
        debugEV << "(29) FSM State BACKOFF, EV_BROADCAST_RECEIVED:"
        << "sending frame up and resuming normal operation." <<endl;
        sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
        delete msg;
        break;
    default:
        fsmErrorPCA(event, msg);
        break;
    }
}



void D2MACWuR::updateStatusCCAPCA(t_mac_event_pca event, cMessage *msg) {
    switch (event) {
    case EV_TIMER_CCA_PCA:
    {
        EV<< "(25) FSM State CCA_3, EV_TIMER_CCA" << endl;
        bool isIdle = phy->getChannelState().isIdle();
        if(isIdle) {
            if (TB == 0) {
            EV << "(3) FSM State CCA_3, EV_TIMER_CCA, [Channel Idle]: -> TRANSMITFRAME_4." << endl;
            updateMacStatePCA(TRANSMITFRAME_4_PCA);
            phy->setRadioState(MiximRadio::TX);
            macpkt_ptr_t mac = check_and_cast<macpkt_ptr_t>(macQueue.front()->dup());

            /*macpkt_ptr_t mac = static_cast<macpkt_ptr_t>(macQueue.front());
                          macQueue.pop_front();*/

            attachSignal(mac, simTime()+aTurnaroundTime);
            //sendDown(msg);
            // give time for the radio to be in Tx state before transmitting
            sendDelayed(mac, aTurnaroundTime, lowerLayerOut);
            EV << "Message Name=" << mac->getName() << " and Message Type= " <<  mac->getKind() << "\n";
            EV << "WuC PCA sent to phy layer " << endl;
            nbTxWuC++;
            //Nouveau code
            //cMessage * m=macQueue.front();
            cMessage * m = new cMessage;
            m->setName("WUC_START_TX");
            m->setKind(WUC_START_TX);
            sendControlUp(m);
            }else{
            EV << "(7) FSM State CCA_3, EV_TIMER_CCA, [TB#0]: "
             << " redo backoff." << endl;

              TB=TB-1;
              EV << "TB= "<< TB << endl;

              // redo backoff
              updateMacStatePCA(BACKOFF_2_PCA);
              scheduleAt(simTime(), backoffTimerPCA);
              /*delayBackoff= aUnitBackoffPeriod - ccaDetectionTime;
              EV << "delayBackoff: "
               << delayBackoff << endl;*/
              //delay=delay+delayBackoff;
                    // delay=delay+delayBackoff;
                   //  EV <<" Delay =" << delay << endl;

              elapsedTime= msg->getArrivalTime()- arrivalTime;
              EV <<" elapsedTime =" << elapsedTime << endl;
              delay=delay+elapsedTime;
              EV <<" Delay =" << delay << endl;
               //scheduleAt(simTime()+delayBackoff, backoffTimerPCA);

              // startTimerPCA(TIMER_BACKOFF_PCA);


                }
             } else {

           EV << "(7) FSM State CCA_3, EV_TIMER_CCA, [Channel Busy]: "
         << " redo backoff." << endl;

          // redo backoff channel busy
            updateMacStatePCA(BACKOFF_2_PCA);
            scheduleAt(simTime(), backoffTimerPCA);
           /* delayBackoff= aUnitBackoffPeriod - ccaDetectionTime;
            EV << "delayBackoff: " << delayBackoff << endl;
            delay=delay+delayBackoff;
            EV <<" Delay =" << delay << endl;*/

            elapsedTime= msg->getArrivalTime()- arrivalTime;
            EV <<" elapsedTime =" << elapsedTime << endl;
            delay=delay+elapsedTime;
            EV <<" Delay =" << delay << endl;

            // scheduleAt(simTime()+delayBackoff, backoffTimerPCA);


            //startTimerPCA(TIMER_BACKOFF_PCA);
           // Channel was busy, increment 802.15.4 backoff timers as specified.

                        }
        break;
    }
    case EV_DUPLICATE_RECEIVED_PCA:
        EV << "(26) FSM State CCA_3, EV_DUPLICATE_RECEIVED:";
        if(useMACAcks) {
            EV << " setting up radio tx -> WAITSIFS." << endl;
            // suspend current transmission attempt,
            // transmit ack,
            // and resume transmission when entering manageQueue()
            transmissionAttemptInterruptedByRx = true;
            cancelEvent(ccaTimer);

            phy->setRadioState(MiximRadio::TX);
            updateMacStatePCA(WAITSIFS_6_PCA);
            startTimerPCA(TIMER_SIFS_PCA);
        } else {
            EV << " Nothing to do." << endl;
        }
        //sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
        delete msg;
        break;

    case EV_FRAME_RECEIVED_PCA:
        EV << "(26) FSM State CCA_3, EV_FRAME_RECEIVED:";
        if(useMACAcks) {
            EV << " setting up radio tx -> WAITSIFS." << endl;
            // suspend current transmission attempt,
            // transmit ack,
            // and resume transmission when entering manageQueue()
            transmissionAttemptInterruptedByRx = true;
            cancelEvent(ccaTimerPCA);
            phy->setRadioState(MiximRadio::TX);
            updateMacStatePCA(WAITSIFS_6_PCA);
            startTimerPCA(TIMER_SIFS_PCA);
        } else {
            EV << " Nothing to do." << endl;
        }
        sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
        delete msg;
        break;
    case EV_BROADCAST_RECEIVED_PCA:
        EV << "(24) FSM State BACKOFF, EV_BROADCAST_RECEIVED:"
        << " Nothing to do." << endl;
        sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
        delete msg;
        break;
    default:
        fsmErrorPCA(event, msg);
        break;
    }
}

void D2MACWuR::updateStatusTransmitFramePCA(t_mac_event_pca event, cMessage *msg) {
    if (event == EV_FRAME_TRANSMITTED_PCA) {
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
            updateMacStatePCA(WAITACK_5_PCA);
            startTimerPCA(TIMER_RX_ACK_PCA);
        } else {
            EV << ": RadioSetupRx, manageQueue..." << endl;
            macQueue.pop_front();
            delete packet;
            manageQueuePCA();
        }
    } else {
        fsmErrorPCA(event, msg);
    }
}



void D2MACWuR::updateStatusWaitAckPCA(t_mac_event_pca event, cMessage *msg) {
    assert(useMACAcks);

    cMessage * mac;
    switch (event) {
    case EV_ACK_RECEIVED_PCA:
        EV<< "(5) FSM State WAITACK_5, EV_ACK_RECEIVED: "
        << " ProcessAck, manageQueue..." << endl;
        if(rxAckTimerPCA->isScheduled())
        cancelEvent(rxAckTimerPCA);
        mac = static_cast<cMessage *>(macQueue.front());
        macQueue.pop_front();
        txAttempts = 0;
        mac->setName("MAC SUCCESS");
        mac->setKind(TX_OVER);
        sendControlUp(mac);
        delete msg;
        manageQueuePCA();
        break;
    case EV_ACK_TIMEOUT_PCA:
        EV << "(12) FSM State WAITACK_5, EV_ACK_TIMEOUT:"
        << " incrementCounter/dropPacket, manageQueue..." << endl;
        manageMissingAckPCA(event, msg);
        break;
    case EV_BROADCAST_RECEIVED_PCA:
    case EV_FRAME_RECEIVED_PCA:
        sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
        break;
    case EV_DUPLICATE_RECEIVED_PCA:
        EV << "Error ! Received a frame during SIFS !" << endl;
        delete msg;
        break;
    default:
        fsmErrorPCA(event, msg);
        break;
    }

}

void D2MACWuR::manageMissingAckPCA(t_mac_event_pca /*event*/, cMessage */*msg*/) {
    if (txAttempts < macMaxFrameRetries + 1) {
        // increment counter
        txAttempts++;
        EV<< "I will retransmit this packet (I already tried "
        << txAttempts << " times)." << endl;
    } else {
        // drop packet
        EV << "Packet was transmitted " << txAttempts
        << " times and I never got an Ack. I drop the packet." << endl;
        cMessage * mac = macQueue.front();
        macQueue.pop_front();
        txAttempts = 0;
        mac->setName("MAC ERROR");
        mac->setKind(PACKET_DROPPED);
        sendControlUp(mac);
    }
    manageQueuePCA();
}
void D2MACWuR::updateStatusSIFSPCA(t_mac_event_pca event, cMessage *msg) {
    assert(useMACAcks);

    switch (event) {
    case EV_TIMER_SIFS_PCA:
        EV<< "(17) FSM State WAITSIFS_6, EV_TIMER_SIFS:"
        << " sendAck -> TRANSMITACK." << endl;
        updateMacStatePCA(TRANSMITACK_7_PCA);
        attachSignal(ackMessage, simTime());
        sendDown(ackMessage);
        nbTxAcks++;
        //      sendDelayed(ackMessage, aTurnaroundTime, lowerLayerOut);
        ackMessage = NULL;
        break;
    case EV_TIMER_BACKOFF_PCA:
        // Backoff timer has expired while receiving a frame. Restart it
        // and stay here.
        EV << "(16) FSM State WAITSIFS_6, EV_TIMER_BACKOFF. "
        << "Restart backoff timer and don't move." << endl;
        startTimerPCA(TIMER_BACKOFF_PCA);
        break;
    case EV_BROADCAST_RECEIVED_PCA:
    case EV_FRAME_RECEIVED_PCA:
        EV << "Error ! Received a frame during SIFS !" << endl;
        sendUp(decapsMsg(static_cast<macpkt_ptr_t>(msg)));
        delete msg;
        break;
    default:
        fsmErrorPCA(event, msg);
        break;
    }
}

void D2MACWuR::updateStatusTransmitAckPCA(t_mac_event_pca event, cMessage *msg) {
    assert(useMACAcks);

    if (event == EV_FRAME_TRANSMITTED_PCA) {
        EV<< "(19) FSM State TRANSMITACK_7, EV_FRAME_TRANSMITTED:"
        << " ->manageQueue." << endl;
        phy->setRadioState(MiximRadio::RX);
        //      delete msg;
        manageQueuePCA();
    } else {
        fsmErrorPCA(event, msg);
    }
}




void D2MACWuR::updateStatusNotIdlePCA(cMessage *msg) {
    EV<< "(20) FSM State NOT IDLE, EV_SEND_REQUEST. Is a TxBuffer available ?" << endl;
    if (macQueue.size() <= queueLength) {
        macQueue.push_back(static_cast<macpkt_ptr_t>(msg));
        EV << "(21) FSM State NOT IDLE, EV_SEND_REQUEST"
        <<" and [TxBuff avail]: enqueue packet and don't move." << endl;
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
    }

}
/**
 * Updates state machine.
 */
void D2MACWuR::executeMacPCA(t_mac_event_pca event, cMessage *msg) {
    EV<< "In executeMac" << endl;
    EV<< "macStatePCA= " <<macStatePCA <<endl;
    if(macStatePCA != IDLE_1_PCA && event == EV_SEND_REQUEST_PCA) {
        updateStatusNotIdlePCA(msg);
    } else {
        switch(macStatePCA) {
        case IDLE_1_PCA:
            updateStatusIdlePCA(event, msg);
            break;
        case BACKOFF_2_PCA:
            updateStatusBackoffPCA(event, msg);
            break;
        case CCA_3_PCA:
            updateStatusCCAPCA(event, msg);
            break;
        case TRANSMITFRAME_4_PCA:
            updateStatusTransmitFramePCA(event, msg);
            break;
        case WAITACK_5_PCA:
            updateStatusWaitAckPCA(event, msg);
            break;
        case WAITSIFS_6_PCA:
            updateStatusSIFSPCA(event, msg);
            break;
        case TRANSMITACK_7_PCA:
            updateStatusTransmitAckPCA(event, msg);
            break;
        default:
            EV << "Error in CSMA FSM: an unknown state has been reached. macStatePCA=" << macStatePCA << endl;
            break;
        }
    }
}

void D2MACWuR::manageQueuePCA() {
    if (macQueue.size() != 0) {
        EV<< "(manageQueue) there are " << macQueue.size() << " packets to send, entering backoff wait state." << endl;
        if( transmissionAttemptInterruptedByRx) {
            // resume a transmission cycle which was interrupted by
            // a frame reception during CCA check
            transmissionAttemptInterruptedByRx = false;
        } /*else {
            // initialize counters if we start a new transmission
            // cycle from zero
            //delay = 0;
            //BE = macMinBE;
        }*/
      /* if(! backoffTimerPCA->isScheduled()) {
          //startTimer(TIMER_BACKOFF);
            scheduleAt(simTime(), backoffTimerPCA);
        }
       updateMacStatePCA(BACKOFF_2_PCA);*/
        //updateMacStatePCA(IDLE_1_PCA);
    } else {
        EV << "(manageQueue) no packets to send, entering IDLE state." << endl;
        updateMacStatePCA(IDLE_1_PCA);
    }
}

void D2MACWuR::updateMacStatePCA(t_mac_states_pca newMacState) {
    macStatePCA = newMacState;
}

/*
 * Called by the FSM machine when an unknown transition is requested.
 */
void D2MACWuR::fsmErrorPCA(t_mac_event_pca event, cMessage *msg) {
    EV<< "FSM Error ! In state " << macState << ", received unknown event:" << event << "." << endl;
    if (msg != NULL)
    delete msg;
}

void D2MACWuR::startTimerPCA(t_mac_timer_pca timer) {
    /*if (timer == TIMER_BACKOFF) {
        scheduleAt(scheduleBackoff(), backoffTimer);
    } else */ if (timer == TIMER_CCA_PCA) {
        simtime_t ccaTime = rxSetupTime + ccaDetectionTime;
        EV<< "(startTimer) ccaTimer value=" << ccaTime
        << "(rxSetupTime,ccaDetectionTime:" << rxSetupTime
        << "," << ccaDetectionTime <<")." << endl;
        scheduleAt(simTime()+rxSetupTime+ccaDetectionTime, ccaTimerPCA);
    } else if (timer==TIMER_SIFS_PCA) {
        assert(useMACAcks);
        debugEV << "(startTimer) sifsTimer value=" << sifs << endl;
        scheduleAt(simTime()+sifs, sifsTimerPCA);
    } else if (timer==TIMER_RX_ACK_PCA) {
        assert(useMACAcks);
        debugEV << "(startTimer) rxAckTimer value=" << macAckWaitDuration << endl;
        scheduleAt(simTime()+macAckWaitDuration, rxAckTimerPCA);
    } else {
        EV << "Unknown timer requested to start:" << timer << endl;
    }
}



/**
 * Compares the address of this Host with the destination address in
 * frame. Generates the corresponding event.
 */
