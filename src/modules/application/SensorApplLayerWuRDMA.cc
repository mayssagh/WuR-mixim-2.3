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

#include "SensorApplLayerWuRDMA.h"

#include <sstream>

#include "BaseNetwLayer.h"
#include "AddressingInterface.h"
#include "NetwControlInfo.h"
#include "FindModule.h"
#include "SimpleAddress.h"
#include "BaseWorldUtility.h"
#include "ApplPkt_m.h"
#include "IControl.h"
#include "BaseMacLayer.h"

Define_Module(SensorApplLayerWuRDMA);

void SensorApplLayerWuRDMA::initialize(int stage) {
    BaseLayer::initialize(stage);
    if (stage == 0) {
        BaseLayer::catPacketSignal.initialize();

        debugEV<< "in initialize() stage 0...";
        debug = par("debug");
        stats = par("stats");
        trace = par("trace");
        nbPacketsCSMA = par("nbPacketsCSMA");
        nbPacketsPCA = par("nbPacketsPCA");
        trafficParam = par("trafficParam");
        initializationTime = par("initializationTime");
        broadcastPackets = par("broadcastPackets");
        headerLength = par("headerLength");
        packetLength = par ("packetLength");
        WuCLength = par ("WuCLength");
        WuCTime = par ("WuCTime");
        TimeON = par ("TimeON");

        // application configuration
        const char *traffic = par("trafficType");
        destAddr = LAddress::L3Type(par("destAddr").longValue());
        nbPacketsSentCSMA = 0;
        nbPacketsReceivedCSMA = 0;
        firstPacketGenerationCSMA = -1;
        lastPacketReceptionCSMA = -2;

        nbPacketsSentPCA = 0;
        nbPacketsReceivedPCA = 0;
        firstPacketGenerationPCA = -1;
        lastPacketReceptionPCA = -2;

        nbWuCsSentCSMA = 0;
        nbWuCsReceivedCSMA = 0;
        firstWuCGenerationCSMA = -1;
        lastWuCReceptionCSMA = -2;

        nbWuCsSentPCA = 0;
        nbWuCsReceivedPCA = 0;
        firstWuCGenerationPCA = -1;
        lastWuCReceptionPCA = -2;

        initializeDistribution(traffic);

        delayTimer = new cMessage("appDelay", SEND_DATA_TIMER);
        delayTimerCSMA = new cMessage("appDelayCSMA", SEND_DATA_TIMER_CSMA);

        // get pointer to the world module
        world = FindModule<BaseWorldUtility*>::findGlobalModule();

    } else if (stage == 1) {
        debugEV << "in initialize() stage 1...";
        // Application address configuration: equals to host address

        cModule *const pHost = findHost();
        const cModule* netw  = FindModule<BaseNetwLayer*>::findSubModule(pHost);
        if(!netw) {
            netw = pHost->getSubmodule("netwl");
            if(!netw) {
                opp_error("Could not find network layer module. This means "
                          "either no network layer module is present or the "
                          "used network layer module does not subclass from "
                          "BaseNetworkLayer.");
            }
        }
        const AddressingInterface *const addrScheme = FindModule<AddressingInterface*>::findSubModule(pHost);
        if(addrScheme) {
            myAppAddr = addrScheme->myNetwAddr(netw);
        } else {
            myAppAddr = LAddress::L3Type( netw->getId() );
        }
        sentPacketsCSMA = 0;
        sentPacketsPCA = 0;
        sentWuCCSMA = 0;
        sentWuCPCA = 0;

        // first packet generation time is always chosen uniformly
        // to avoid systematic collisions
        if(nbPacketsCSMA> 0)
            scheduleAt(simTime() +uniform(initializationTime, initializationTime + trafficParam), delayTimerCSMA);
       // if(nbPacketsPCA> 0)
            // scheduleAt(simTime() +uniform(initializationTime, initializationTime + trafficParam), delayTimerCSMA);


        if (stats) {
            latenciesRawCSMA.setName("rawLatencies");
            latenciesRawCSMA.setUnit("s");
            latencyCSMA.setName("latency");

            latenciesRawPCA.setName("rawLatencies");
            latenciesRawPCA.setUnit("s");
            latencyPCA.setName("latency");

            latenciesRawWuCCSMA.setName("rawLatenciesWuCCSMA");
            latenciesRawWuCCSMA.setUnit("s");
            latencyWuCCSMA.setName("latencyWuCCSMA");

            latenciesRawWuCPCA.setName("rawLatenciesWuCPCA");
            latenciesRawWuCPCA.setUnit("s");
            latencyWuCPCA.setName("latencyWuCPCA");

            latenciesRawACK.setName("rawLatenciesACK");
            latenciesRawACK.setUnit("s");
            latencyACK.setName("latencyACK");
        }
    }
}

cStdDev& SensorApplLayerWuRDMA::hostsLatency(const LAddress::L3Type& hostAddress)
{
    using std::pair;

    if(latenciesCSMA.count(hostAddress) == 0) {
        std::ostringstream oss;
        oss << hostAddress;
        cStdDev aLatencyCSMA(oss.str().c_str());
        latenciesCSMA.insert(pair<LAddress::L3Type, cStdDev>(hostAddress, aLatencyCSMA));
    }

    return latenciesCSMA[hostAddress];

    if(latenciesPCA.count(hostAddress) == 0) {
       std::ostringstream oss;
       oss << hostAddress;
       cStdDev aLatencyPCA(oss.str().c_str());
       latenciesPCA.insert(pair<LAddress::L3Type, cStdDev>(hostAddress, aLatencyPCA));
        }

        return latenciesPCA[hostAddress];

    if(latenciesWuCCSMA.count(hostAddress) == 0) {
       std::ostringstream oss;
       oss << hostAddress;
       cStdDev aLatencyCSMA(oss.str().c_str());
       latenciesWuCCSMA.insert(pair<LAddress::L3Type, cStdDev>(hostAddress, aLatencyCSMA));
        }

    return latenciesWuCCSMA[hostAddress];

    if(latenciesWuCPCA.count(hostAddress) == 0) {
      std::ostringstream oss;
      oss << hostAddress;
      cStdDev aLatencyPCA(oss.str().c_str());
      latenciesWuCPCA.insert(pair<LAddress::L3Type, cStdDev>(hostAddress, aLatencyPCA));
        }

    return latenciesWuCPCA[hostAddress];

    if(latenciesACK.count(hostAddress) == 0) {
      std::ostringstream oss;
      oss << hostAddress;
      cStdDev aLatencyACK(oss.str().c_str());
      latenciesACK.insert(pair<LAddress::L3Type, cStdDev>(hostAddress, aLatencyACK));
        }

    return latenciesACK[hostAddress];
}

void SensorApplLayerWuRDMA::initializeDistribution(const char* traffic) {
    if (!strcmp(traffic, "periodic")) {
        trafficType = PERIODIC;
    } else if (!strcmp(traffic, "uniform")) {
        trafficType = UNIFORM;
    } else if (!strcmp(traffic, "exponential")) {
        trafficType = EXPONENTIAL;
    } else {
        trafficType = UNKNOWN;
        EV << "Error! Unknown traffic type: " << traffic << endl;
    }
}

void SensorApplLayerWuRDMA::scheduleNextPacket() {
   // if (nbPacketsPCA > sentWuCPCA && trafficType != 0) { // We must generate packets
    if (nbPacketsPCA > sentPacketsPCA && trafficType != 0) {
        simtime_t waitTime = SIMTIME_ZERO;

        switch (trafficType) {
        case PERIODIC:
            waitTime = trafficParam;
            debugEV<< "Periodic traffic, waitTime=" << waitTime << endl;
            break;
            case UNIFORM:
            waitTime = uniform(0, trafficParam);
            debugEV << "Uniform traffic, waitTime=" << waitTime << endl;
            break;
            case EXPONENTIAL:
            waitTime = exponential(trafficParam);
            debugEV << "Exponential traffic, waitTime=" << waitTime << endl;
            break;
            case UNKNOWN:
            default:
            EV <<
            "Cannot generate requested traffic type (unimplemented or unknown)."
            << endl;
            return; // don not schedule
            break;
        }
        EV << "Start timer for a new packet in " << waitTime << " seconds." <<
        endl;
        scheduleAt(simTime() + waitTime, delayTimer);
        EV << "...timer rescheduled." << endl;
    } else {
        //scheduleAt(simTime() +uniform(initializationTime, initializationTime + trafficParam), delayTimerCSMA);
        debugEV << "All packets sent.\n";
    }
}

void SensorApplLayerWuRDMA::scheduleNextPacketCSMA() {
   //if (nbPacketsCSMA > sentWuCCSMA && trafficType != 0) { // We must generate packets sentPacketsPCA
    if (nbPacketsCSMA > sentPacketsCSMA && trafficType != 0) {
        simtime_t waitTime = SIMTIME_ZERO;

        switch (trafficType) {
        case PERIODIC:
            waitTime = trafficParam;
            debugEV<< "Periodic traffic, waitTime=" << waitTime << endl;
            break;
            case UNIFORM:
            waitTime = uniform(0, trafficParam);
            debugEV << "Uniform traffic, waitTime=" << waitTime << endl;
            break;
            case EXPONENTIAL:
            waitTime = exponential(trafficParam);
            debugEV << "Exponential traffic, waitTime=" << waitTime << endl;
            break;
            case UNKNOWN:
            default:
            EV <<
            "Cannot generate requested traffic type (unimplemented or unknown)."
            << endl;
            return; // don not schedule
            break;
        }
        EV << "Start timer for a new packet in " << waitTime << " seconds." <<
        endl;
        scheduleAt(simTime() + waitTime, delayTimerCSMA);
        EV << "...timer rescheduled." << endl;
    } else {
        //scheduleAt(simTime() +uniform(initializationTime, initializationTime + trafficParam), delayTimerPCA);
        scheduleAt(simTime() +uniform(initializationTime, initializationTime + trafficParam), delayTimer);
        //debugEV << "All packets sent.\n";
    }
}

/**
 * @brief Handling of messages arrived to destination
 **/
void SensorApplLayerWuRDMA::handleLowerMsg(cMessage * msg) {
    ApplPkt *m;

    switch (msg->getKind()) {
    case WUC_MESSAGE_CSMA:
        m = static_cast<ApplPkt *> (msg);
        nbWuCsReceivedCSMA++;
        packet.setPacketSent(false);
        packet.setNbPacketsSent(0);
        packet.setNbPacketsReceived(1);
        packet.setHost(myAppAddr);
        emit(BaseLayer::catPacketSignal, &packet);
        if (stats) {
          simtime_t theLatencyWuCCSMA = m->getArrivalTime() - m->getCreationTime();
          if(trace) {
             hostsLatency(m->getSrcAddr()).collect(theLatencyWuCCSMA);
             latenciesRawWuCCSMA.record(SIMTIME_DBL(theLatencyWuCCSMA));
                    }
             latencyWuCCSMA.collect(theLatencyWuCCSMA);
             if (firstWuCGenerationCSMA < 0)
                firstWuCGenerationCSMA = m->getCreationTime();
                lastWuCReceptionCSMA = m->getArrivalTime();
                if(trace) {
                  EV<< "Received a WUC packet CSMA from host[" << m->getSrcAddr()
                   << "], latencyWuCCSMA=" << theLatencyWuCCSMA
                   << ", collected " << hostsLatency(m->getSrcAddr()).
                   getCount() << "mean is now: " << hostsLatency(m->getSrcAddr()).
                   getMean() << endl;
                } else {
                   EV<< "Received a WUC packet CSMA from host[" << m->getSrcAddr()
                   << "], latency=" << theLatencyWuCCSMA << endl;
                    }
                }
                delete msg;

                //  sendReply(m);
                break;
    case WUC_MESSAGE_PCA:
        m = static_cast<ApplPkt *> (msg);
        nbWuCsReceivedPCA++;
        packet.setPacketSent(false);
        packet.setNbPacketsSent(0);
        packet.setNbPacketsReceived(1);
        packet.setHost(myAppAddr);
        emit(BaseLayer::catPacketSignal, &packet);
        if (stats) {
         simtime_t theLatencyWuCPCA = m->getArrivalTime() - m->getCreationTime();
              if(trace) {
                 hostsLatency(m->getSrcAddr()).collect(theLatencyWuCPCA);
                 latenciesRawWuCPCA.record(SIMTIME_DBL(theLatencyWuCPCA));
                        }
                 latencyWuCPCA.collect(theLatencyWuCPCA);
                 if (firstWuCGenerationPCA < 0)
                    firstWuCGenerationPCA = m->getCreationTime();
                    lastWuCReceptionPCA = m->getArrivalTime();
                    if(trace) {
                      EV<< "Received a WuC PCA  from host[" << m->getSrcAddr()
                       << "], latencyWuCPCA=" << theLatencyWuCPCA
                       << ", collected " << hostsLatency(m->getSrcAddr()).
                       getCount() << "mean is now: " << hostsLatency(m->getSrcAddr()).
                       getMean() << endl;
                    } else {
                       EV<< "Received a WuC packet PCA from host[" << m->getSrcAddr()
                       << "], latencyWuCPCA=" << theLatencyWuCPCA << endl;
                        }
                    }
                    delete msg;

                    //  sendReply(m);
                    break;

    case DATA_MESSAGE_CSMA:
        m = static_cast<ApplPkt *> (msg);
        nbPacketsReceivedCSMA++;
        packet.setPacketSent(false);
        packet.setNbPacketsSent(0);
        packet.setNbPacketsReceived(1);
        packet.setHost(myAppAddr);
        emit(BaseLayer::catPacketSignal, &packet);
        if (stats) {
            simtime_t theLatencyCSMA = m->getArrivalTime() - m->getCreationTime();
            if(trace) {
              hostsLatency(m->getSrcAddr()).collect(theLatencyCSMA);
              latenciesRawCSMA.record(SIMTIME_DBL(theLatencyCSMA));
            }
            latencyCSMA.collect(theLatencyCSMA);
            if (firstPacketGenerationCSMA < 0)
                firstPacketGenerationCSMA = m->getCreationTime();
            lastPacketReceptionCSMA = m->getArrivalTime();
            if(trace) {
              EV<< "Received a data packet from host[" << m->getSrcAddr()
              << "], latencyCSMA =" << theLatencyCSMA
              << ", collected " << hostsLatency(m->getSrcAddr()).
              getCount() << "mean is now: " << hostsLatency(m->getSrcAddr()).
              getMean() << endl;
            } else {
                  EV<< "Received a data packet from host[" << m->getSrcAddr()
                  << "], latency=" << theLatencyCSMA << endl;
            }
        }
        delete msg;

        //  sendReply(m);
        break;
    case DATA_MESSAGE:
            m = static_cast<ApplPkt *> (msg);
            nbPacketsReceivedPCA++;
            packet.setPacketSent(false);
            packet.setNbPacketsSent(0);
            packet.setNbPacketsReceived(1);
            packet.setHost(myAppAddr);
            emit(BaseLayer::catPacketSignal, &packet);
            if (stats) {
                simtime_t theLatencyPCA = m->getArrivalTime() - m->getCreationTime();
                if(trace) {
                  hostsLatency(m->getSrcAddr()).collect(theLatencyPCA);
                  latenciesRawPCA.record(SIMTIME_DBL(theLatencyPCA));
                }
                latencyPCA.collect(theLatencyPCA);
                if (firstPacketGenerationPCA < 0)
                firstPacketGenerationPCA = m->getCreationTime();
                lastPacketReceptionPCA = m->getArrivalTime();
                if(trace) {
                  EV<< "Received a data packet from host[" << m->getSrcAddr()
                  << "], latencyPCA =" << theLatencyPCA
                  << ", collected " << hostsLatency(m->getSrcAddr()).
                  getCount() << "mean is now: " << hostsLatency(m->getSrcAddr()).
                  getMean() << endl;
                } else {
                      EV<< "Received a data packet PCA from host[" << m->getSrcAddr()
                      << "], latencyPCA=" << theLatencyPCA << endl;
                }
            }
            delete msg;

            //  sendReply(m);
            break;
    case ACK:
            m = static_cast<ApplPkt *> (msg);
            nbACKsReceived++;
            packet.setPacketSent(false);
            packet.setNbPacketsSent(0);
            packet.setNbPacketsReceived(1);
            packet.setHost(myAppAddr);
            emit(BaseLayer::catPacketSignal, &packet);
            if (stats) {
                simtime_t theLatencyACK = m->getArrivalTime() - m->getCreationTime();
                if(trace) {
                  hostsLatency(m->getSrcAddr()).collect(theLatencyACK);
                  latenciesRawACK.record(SIMTIME_DBL(theLatencyACK));
                }
                latencyACK.collect(theLatencyACK);
                if (firstACKGeneration < 0)
                    firstACKGeneration = m->getCreationTime();
                    lastACKReception = m->getArrivalTime();
                if(trace) {
                  EV<< "Received an ACK from host[" << m->getSrcAddr()
                  << "], latencyACK =" << theLatencyACK
                  << ", collected " << hostsLatency(m->getSrcAddr()).
                  getCount() << "mean is now: " << hostsLatency(m->getSrcAddr()).
                  getMean() << endl;
                } else {
                      EV<< "Received an ACK packet from host[" << m->getSrcAddr()
                      << "], latency=" << theLatencyACK << endl;
                }
            }
            delete msg;

            //  sendReply(m);
            break;
    case ACK_CSMA:
                m = static_cast<ApplPkt *> (msg);
                nbACKsReceived++;
                packet.setPacketSent(false);
                packet.setNbPacketsSent(0);
                packet.setNbPacketsReceived(1);
                packet.setHost(myAppAddr);
                emit(BaseLayer::catPacketSignal, &packet);
                if (stats) {
                    simtime_t theLatencyACK = m->getArrivalTime() - m->getCreationTime();
                    if(trace) {
                      hostsLatency(m->getSrcAddr()).collect(theLatencyACK);
                      latenciesRawACK.record(SIMTIME_DBL(theLatencyACK));
                    }
                    latencyACK.collect(theLatencyACK);
                    if (firstACKGeneration < 0)
                        firstACKGeneration = m->getCreationTime();
                        lastACKReception = m->getArrivalTime();
                    if(trace) {
                      EV<< "Received an ACK from host[" << m->getSrcAddr()
                      << "], latencyACK =" << theLatencyACK
                      << ", collected " << hostsLatency(m->getSrcAddr()).
                      getCount() << "mean is now: " << hostsLatency(m->getSrcAddr()).
                      getMean() << endl;
                    } else {
                          EV<< "Received an ACK packet from host[" << m->getSrcAddr()
                          << "], latency=" << theLatencyACK << endl;
                    }
                }
                delete msg;

                //  sendReply(m);
        break;
        default:
        EV << "Error! got packet with unknown kind: " << msg->getKind() << endl;
        delete msg;
        break;
    }
}

/**
 * @brief A timer with kind = SEND_DATA_TIMER indicates that a new
 * data has to be send (@ref sendData).
 *
 * There are no other timers implemented for this module.
 *
 * @sa sendData
 **/
void SensorApplLayerWuRDMA::handleSelfMsg(cMessage * msg) {
    switch (msg->getKind()) {
    case SEND_DATA_TIMER_CSMA:
        sendWuCCSMA();
        //sendWuCPCA();
        //delete msg;
        break;
    case SEND_DATA_TIMER:
          sendWuCPCA();
       // sendWuCCSMA();
            //delete msg;
            break;
    default:
        EV<< "Unkown selfmessage! -> delete, kind: " << msg->getKind() << endl;
        delete msg;
        break;
    }
}

void SensorApplLayerWuRDMA::handleLowerControl(cMessage * msg) {
    switch (msg->getKind()) {

        case BaseMacLayer::WUC_START_TX_CSMA:
            if (myAppAddr != 0){
                send (new cMessage("POWER_ON_CSMA", IControl::POWER_ON_CSMA),lowerControlOut);
                 EV <<"WUC_START_TX_CSMA received and TURN_ON_CSMA sent to down layer";
                 sendDataCSMA();
                // scheduleNextPacketCSMA();
                 }
                 break;
        case IControl::AWAKE_CSMA:
            EV<< "Received Control MSG: " << msg->getName() << endl;
            if (myAppAddr != 0){
              //  sendDataCSMA();
                EV <<"sendDataCSMA()";

                 }
            break;
        case IControl::SLEEPING_CSMA:
            EV <<"sentWuCCSMA: "<< sentWuCCSMA << endl;
              if (myAppAddr != 0){
                 // if (nbPacketsCSMA > sentWuCCSMA){
                  //scheduleNextPacketCSMA();
                  //scheduleAt(simTime() + trafficParam, delayTimerPCA);
                 // }
                 // else{
                 // scheduleAt(simTime() +uniform(initializationTime, initializationTime + trafficParam), delayTimerPCA);
                                    //}
              }
              break;
        case BaseMacLayer::WUC_START_TX:
            if (myAppAddr != 0){
            send (new cMessage("POWER_ON_PCA", IControl::POWER_ON_PCA),lowerControlOut);
            EV <<"WUC_START_TX send to up layer";
            sendDataPCA();
            //scheduleNextPacket();

            }
            break;
        case IControl::AWAKE_PCA:
            EV<< "Received Control MSG: " << msg->getName() << endl;
            if (myAppAddr != 0){
           // sendDataPCA();

            }
            break;
        case IControl::SLEEPING_PCA:
           EV <<"sentWuCsPCA: "<< sentWuCPCA << endl;
           if (myAppAddr != 0){
               //if (nbPacketsPCA > sentWuCPCA){
                  // scheduleAt(simTime() + trafficParam, delayTimerPCA);
                 // scheduleNextPacket();
              // }

           }
           break;
        default:
                EV<< "Unkown Controlmessage! -> delete, kind: " << msg->getKind() << endl;
                delete msg;

        }
        // delete msg;
}

/**
  * @brief This function creates a new data message and sends it down to
  * the network layer
 **/
void SensorApplLayerWuRDMA::sendWuCCSMA() {
    ApplPkt *pkt = new ApplPkt("WUC_MESSAGE_CSMA", WUC_MESSAGE_CSMA);

    if(broadcastPackets) {
        pkt->setDestAddr(LAddress::L3BROADCAST);
    } else {
        pkt->setDestAddr(destAddr);
    }
    pkt->setSrcAddr(myAppAddr);
    pkt->setByteLength(WuCLength);
    // set the control info to tell the network layer the layer 3 address
    NetwControlInfo::setControlInfo(pkt, pkt->getDestAddr());
    EV<< "Sending WuC packet CSMA!\n";
    sendDown(pkt);
    nbWuCsSentCSMA++;
    packet.setPacketSent(true);
    packet.setNbPacketsSent(1);
    packet.setNbPacketsReceived(0);
    packet.setHost(myAppAddr);
    emit(BaseLayer::catPacketSignal, &packet);
    sentWuCCSMA++;
   // sendDataCSMA();

    //scheduleNextPacket();
}
void SensorApplLayerWuRDMA::sendWuCPCA() {
    ApplPkt *pkt = new ApplPkt("WUC_MESSAGE_PCA", WUC_MESSAGE_PCA);

    if(broadcastPackets) {
        pkt->setDestAddr(LAddress::L3BROADCAST);
    } else {
        pkt->setDestAddr(destAddr);
    }
    pkt->setSrcAddr(myAppAddr);
    pkt->setByteLength(WuCLength);
    // set the control info to tell the network layer the layer 3 address
    NetwControlInfo::setControlInfo(pkt, pkt->getDestAddr());
    EV<< "Sending WuC packet PCA!\n";
    sendDown(pkt);
    nbWuCsSentPCA++;
    packet.setPacketSent(true);
    packet.setNbPacketsSent(1);
    packet.setNbPacketsReceived(0);
    packet.setHost(myAppAddr);
    emit(BaseLayer::catPacketSignal, &packet);
    sentWuCPCA++;



}
void SensorApplLayerWuRDMA::sendDataCSMA() {
    ApplPkt *pkt = new ApplPkt("DATA_MESSAGE_CSMA", DATA_MESSAGE_CSMA);

    if(broadcastPackets) {
        pkt->setDestAddr(LAddress::L3BROADCAST);
    } else {
        pkt->setDestAddr(destAddr);
    }
    pkt->setSrcAddr(myAppAddr);
    pkt->setByteLength(packetLength);
    // set the control info to tell the network layer the layer 3 address
    NetwControlInfo::setControlInfo(pkt, pkt->getDestAddr());
    EV<< "Sending data packet CSMA!\n";
    //sendDown(pkt);
    sendDelayed(pkt, WuCTime + TimeON, lowerLayerOut);
   //sendDelayed(pkt,  WuCTime, lowerLayerOut);
    //sendDelayed(pkt, TimeON, lowerLayerOut);
    nbPacketsSentCSMA++;
    packet.setPacketSent(true);
    packet.setNbPacketsSent(1);
    packet.setNbPacketsReceived(0);
    packet.setHost(myAppAddr);
    emit(BaseLayer::catPacketSignal, &packet);
    sentPacketsCSMA++;
    scheduleNextPacketCSMA();
   //scheduleNextPacket();
}

void SensorApplLayerWuRDMA::sendDataPCA() {
    ApplPkt *pkt = new ApplPkt("DATA_MESSAGE", DATA_MESSAGE);

    if(broadcastPackets) {
        pkt->setDestAddr(LAddress::L3BROADCAST);
    } else {
        pkt->setDestAddr(destAddr);
    }
    pkt->setSrcAddr(myAppAddr);
    pkt->setByteLength(packetLength);
    // set the control info to tell the network layer the layer 3 address
    NetwControlInfo::setControlInfo(pkt, pkt->getDestAddr());
   EV<< "Sending data packet PCA!\n";
   // sendDown(pkt);
    sendDelayed(pkt, WuCTime + TimeON, lowerLayerOut);
   //sendDelayed(pkt,  WuCTime, lowerLayerOut);
   //sendDelayed(pkt, TimeON, lowerLayerOut);
    nbPacketsSentPCA++;
    packet.setPacketSent(true);
    packet.setNbPacketsSent(1);
    packet.setNbPacketsReceived(0);
    packet.setHost(myAppAddr);
    emit(BaseLayer::catPacketSignal, &packet);
    sentPacketsPCA++;
    scheduleNextPacket();
}


void SensorApplLayerWuRDMA::finish() {
    using std::map;
    if (stats) {
        if (trace) {
            std::stringstream osToStr(std::stringstream::out);
            // output logs to scalar file
            for (map<LAddress::L3Type, cStdDev>::iterator it = latenciesCSMA.begin(); it != latenciesCSMA.end(); ++it) {
                cStdDev aLatencyCSMA = it->second;

                osToStr.str(""); osToStr << "latencyCSMA" << it->first;
                recordScalar(osToStr.str().c_str(), aLatencyCSMA.getMean(), "s");
                aLatencyCSMA.record();
            }

            for (map<LAddress::L3Type, cStdDev>::iterator it = latenciesPCA.begin(); it != latenciesPCA.end(); ++it) {
                cStdDev aLatencyPCA = it->second;

                osToStr.str(""); osToStr << "latencyPCA" << it->first;
                recordScalar(osToStr.str().c_str(), aLatencyPCA.getMean(), "s");
                aLatencyPCA.record();
             }
            for (map<LAddress::L3Type, cStdDev>::iterator it = latenciesWuCCSMA.begin(); it != latenciesWuCCSMA.end(); ++it) {
                cStdDev aLatencyWuCCSMA = it->second;

                osToStr.str(""); osToStr << "latency WuC CSMA" << it->first;
                recordScalar(osToStr.str().c_str(), aLatencyWuCCSMA.getMean(), "s");
                aLatencyWuCCSMA.record();
             }
            for (map<LAddress::L3Type, cStdDev>::iterator it = latenciesWuCPCA.begin(); it != latenciesWuCPCA.end(); ++it) {
                 cStdDev aLatencyWuCPCA = it->second;

                 osToStr.str(""); osToStr << "latency WuC PCA" << it->first;
                 recordScalar(osToStr.str().c_str(), aLatencyWuCPCA.getMean(), "s");
                 aLatencyWuCPCA.record();
            }
        }
        recordScalar("activity duration CSMA", lastPacketReceptionCSMA
                - firstPacketGenerationCSMA, "s");
        recordScalar("firstPacketGenerationCSMA", firstPacketGenerationCSMA, "s");
        recordScalar("lastPacketReceptionCSMA", lastPacketReceptionCSMA, "s");
        recordScalar("nbPacketsSentCSMA", nbPacketsSentCSMA);
        recordScalar("nbPacketsReceivedCSMA", nbPacketsReceivedCSMA);
        latencyCSMA.record();

        recordScalar("activity duration PCA", lastPacketReceptionPCA
                        - firstPacketGenerationPCA, "s");
        recordScalar("firstPacketGenerationPCA", firstPacketGenerationPCA, "s");
        recordScalar("lastPacketReceptionPCA", lastPacketReceptionPCA, "s");
        recordScalar("nbPacketsSentPCA", nbPacketsSentPCA);
        recordScalar("nbPacketsReceivedPCA", nbPacketsReceivedPCA);
        latencyPCA.record();

        recordScalar("activity duration WuC CSMA", lastWuCReceptionCSMA
                        - firstWuCGenerationCSMA, "s");
        recordScalar("firstWuCGenerationCSMA", firstWuCGenerationCSMA, "s");
        recordScalar("lastWuCReceptionCSMA", lastWuCReceptionCSMA, "s");
        recordScalar("nbWuCsSentCSMA", nbWuCsSentCSMA);
        recordScalar("nbWuCsReceivedCSMA", nbWuCsReceivedCSMA);
        latencyWuCCSMA.record();

        recordScalar("activity duration WuC PCA", lastWuCReceptionPCA
                                - firstWuCGenerationPCA, "s");
        recordScalar("firstWuCGenerationPCA", firstWuCGenerationPCA, "s");
        recordScalar("lastWuCReceptionPCA", lastWuCReceptionPCA, "s");
        recordScalar("nbWuCsSentPCA", nbWuCsSentPCA);
        recordScalar("nbWuCsReceivedPCA", nbWuCsReceivedPCA);
        latencyWuCPCA.record();



       recordScalar("lastACKReception", lastACKReception, "s");
       recordScalar("nbACKsReceived", nbACKsReceived);
       latencyACK.record();
    }
    cComponent::finish();
}

SensorApplLayerWuRDMA::~SensorApplLayerWuRDMA() {
    cancelAndDelete(delayTimer);
    cancelAndDelete(delayTimerPCA);
}
