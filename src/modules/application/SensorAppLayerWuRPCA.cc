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

#include "SensorAppLayerWuRPCA.h"
#include <BaseNetwLayer.h>
#include <AddressingInterface.h>
#include "BaseApplLayer.h"
#include "IControl.h"
#include "BaseMacLayer.h"
#include "FindModule.h"

Define_Module(SensorAppLayerWuRPCA);

void SensorAppLayerWuRPCA::initialize(int stage)
{
    BaseLayer::initialize(stage);
        if (stage == 0) {
            BaseLayer::catPacketSignal.initialize();

            EV<< "in initialize() stage 0...";
            debug = par("debug");
            stats = par("stats");
            trace = par("trace");
            nbPackets = par("nbPackets");
            nbWuCPCA = par ("nbWuCPCA");
            trafficParam = par("trafficParam");
            initializationTime = par("initializationTime");
            WuCperiod = par ("WuCperiod");
            beaconTimer = new cMessage( "beacon-timer", BEACON_TIMER);
            broadcastPackets = par("broadcastPackets");
            headerLength = par("headerLength");
            PAYLOAD_SIZE = par("payloadSize"); // data field size
            PAYLOAD_SIZE = PAYLOAD_SIZE * 8; // convert to bits
            // application configuration
            const char *traffic = par("trafficType");
            destAddr = par("destAddr");
            nbPacketsSent = 0;
            nbPacketsSentCSMA = 0;
            nbWuCsSentCSMA=0;
            nbWuCsSentPCA=0;
            nbPacketsReceived = 0;
            nbPacketsReceivedCSMA = 0;
            nbWuCsReceivedCSMA =0;
            nbWuCsReceivedPCA =0;
            nbACKsReceived=0;
            firstWuCGenerationCSMA = -1;
            lastWuCReceptionCSMA = -2;
            firstWuCGenerationPCA = -1;
            lastWuCReceptionPCA = -2;
            firstPacketGeneration = -1;
            lastPacketReception = -2;
            firstACKGeneration = -1;
            lastACKReception = -2;

            initializeDistribution(traffic);

            delayTimer = new cMessage("appDelay", SEND_DATA_TIMER);
            // Blackboard stuff:
            hostID = getParentModule()->getId();

            // get pointer to the world module

            world = FindModule<BaseWorldUtility*>::findGlobalModule();

        } else if (stage == 1) {
            EV << "in initialize() stage 1...";
            // Application address configuration: equals to host address

            cModule *const pHost = findHost();
            const cModule* netw  = FindModule<BaseNetwLayer*>::findSubModule(pHost);
            //cModule *netw = FindModule<BaseNetwLayer*>::findSubModule(findHost());
            if(!netw) {
                netw = findHost()->getSubmodule("netw");
                if(!netw) {
                    opp_error("Could not find network layer module. This means "
                              "either no network layer module is present or the "
                              "used network layer module does not subclass from "
                              "BaseNetworkLayer.");
                }
            }
            AddressingInterface* addrScheme = FindModule<AddressingInterface*>
                                                        ::findSubModule(findHost());
            if(addrScheme) {
                myAppAddr = addrScheme->myNetwAddr(netw);
            } else {
                myAppAddr = netw->getId();
            }
            sentWuCsCSMA=0;
            sentWuCsPCA=0;
            sentPackets = 0;
            sentPacketsPCA =0;
            //catPacket = world->getCategory(&packet);

            // first packet generation time is always chosen uniformly
            // to avoid systematic collisions
            if(nbPackets> 0)
            scheduleAt(simTime() +uniform(initializationTime, initializationTime + trafficParam), delayTimer);

            if (stats) {
                WuClatenciesRawCSMA.setName("WuCrawLatenciesCSMA");
                WuClatenciesRawCSMA.setUnit("s");
                WuClatencyCSMA.setName("WuClatencyCSMA");
                WuClatenciesRawPCA.setName("WuCrawLatenciesPCA");
                WuClatenciesRawPCA.setUnit("s");
                WuClatencyPCA.setName("WuClatencyPCA");
                latenciesRaw.setName("rawLatencies");
                latenciesRaw.setUnit("s");
                latency.setName("latency");
                ACKlatenciesRaw.setName("ACKrawLatencies");
                ACKlatenciesRaw.setUnit("s");
                ACKlatency.setName("ACKlatency");
            }
        }
}

cStdDev& SensorAppLayerWuRPCA::hostsLatency(int hostAddress)
{
      if(latencies.count(hostAddress) == 0) {
          std::ostringstream oss;
          oss << hostAddress;
          cStdDev aLatency(oss.str().c_str());
          latencies.insert(pair<int, cStdDev>(hostAddress, aLatency));
      }
      return latencies[hostAddress];

      if(latenciesCSMA.count(hostAddress) == 0) {
                std::ostringstream oss;
                oss << hostAddress;
                cStdDev aLatencyCSMA(oss.str().c_str());
                latenciesCSMA.insert(pair<int, cStdDev>(hostAddress, aLatencyCSMA));
            }
            return latenciesCSMA[hostAddress];

      if(WuClatenciesCSMA.count(hostAddress) == 0) {
              std::ostringstream oss;
              oss << hostAddress;
              cStdDev WuCaLatencyCSMA(oss.str().c_str());
              WuClatenciesCSMA.insert(pair<int, cStdDev>(hostAddress, WuCaLatencyCSMA));
          }

      return WuClatenciesCSMA[hostAddress];

      if(WuClatenciesPCA.count(hostAddress) == 0) {
                    std::ostringstream oss;
                    oss << hostAddress;
                    cStdDev WuCaLatencyPCA(oss.str().c_str());
                    WuClatenciesPCA.insert(pair<int, cStdDev>(hostAddress, WuCaLatencyPCA));
                }

            return WuClatenciesPCA[hostAddress];

      if(ACKlatencies.count(hostAddress) == 0) {
              std::ostringstream oss;
              oss << hostAddress;
              cStdDev ACKaLatency(oss.str().c_str());
              ACKlatencies.insert(pair<int, cStdDev>(hostAddress, ACKaLatency));
          }
          return ACKlatencies[hostAddress];
}

void SensorAppLayerWuRPCA::initializeDistribution(const char* traffic) {
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

void SensorAppLayerWuRPCA::scheduleNextPacket() {
    if (nbPackets > sentPackets && trafficType != 0) { // We must generate packets
       // if (nbPackets > sentWuCsCSMA && trafficType != 0) {

        simtime_t waitTime = -1;

        switch (trafficType) {
        case PERIODIC:
            waitTime = trafficParam;
            EV<< "Periodic traffic, waitTime=" << waitTime << endl;
            break;
            case UNIFORM:
            waitTime = uniform(0, trafficParam);
            EV << "Uniform traffic, waitTime=" << waitTime << endl;
            break;
            case EXPONENTIAL:
            waitTime = exponential(trafficParam);
            EV << "Exponential traffic, waitTime=" << waitTime << endl;
            break;
            case UNKNOWN:
            default:
            EV <<
            "Cannot generate requested traffic type (unimplemented or unknown)."
            << endl;

        }
        EV << "Start timer for a new packet in " << waitTime << " seconds." <<
        endl;
        //if (delayTimer->isScheduled()) {
        //cancelEvent(delayTimer);
        scheduleAt(simTime() + waitTime, delayTimer);//}
        EV << "...timer rescheduled." << endl;
    } else {
        EV << "All packets sent.\n";
    }

}


void SensorAppLayerWuRPCA::scheduleNextPacketPCA() {
    if (nbWuCPCA > sentPacketsPCA && trafficType != 0) { // We must generate packets
       // if (nbPackets > sentWuCsCSMA && trafficType != 0) {

        simtime_t waitTime = -1;

        switch (trafficType) {
        case PERIODIC:
            waitTime = trafficParam;
            EV<< "Periodic traffic, waitTime=" << waitTime << endl;
            break;
            case UNIFORM:
            waitTime = uniform(0, trafficParam);
            EV << "Uniform traffic, waitTime=" << waitTime << endl;
            break;
            case EXPONENTIAL:
            waitTime = exponential(trafficParam);
            EV << "Exponential traffic, waitTime=" << waitTime << endl;
            break;
            case UNKNOWN:
            default:
            EV <<
            "Cannot generate requested traffic type (unimplemented or unknown)."
            << endl;

        }
        EV << "Start timer for a new packet in " << waitTime << " seconds." <<
        endl;
        //if (delayTimer->isScheduled()) {
        //cancelEvent(delayTimer);
        scheduleAt(simTime() + waitTime, delayTimer);//}
        EV << "...timer rescheduled." << endl;
    } else {
        EV << "All packets sent.\n";
    }

}



/**
 * @brief Handling of messages arrived to destination
 **/
void SensorAppLayerWuRPCA::handleLowerMsg(cMessage * msg) {
    ApplPkt *m;

    switch (msg->getKind()) {
    case WUC_MESSAGE_CSMA:
    m = static_cast<ApplPkt *> (msg);
    nbWuCsReceivedCSMA++;
    packet.setPacketSent(false);
    packet.setNbPacketsSent(0);
    packet.setNbPacketsReceived(1);
    packet.setHost(myAppAddr);
    //world->publishBBItem(catPacket, &packet, hostID);
        if (stats) {
            simtime_t theWuCLatencyCSMA = m->getArrivalTime() - m->getCreationTime();
                    if(trace) {
                      hostsLatency(m->getSrcAddr()).collect(theWuCLatencyCSMA);
                      WuClatenciesRawCSMA.record(theWuCLatencyCSMA.dbl());
                    }
                    WuClatencyCSMA.collect(theWuCLatencyCSMA);
                    if (firstWuCGenerationCSMA < 0)
                        firstWuCGenerationCSMA = m->getCreationTime();
                    lastWuCReceptionCSMA = m->getArrivalTime();
                    if(trace) {
                      EV<< "Received a WuC (CSMA) packet from host[" << m->getSrcAddr()
                      << "], latency=" << theWuCLatencyCSMA
                      << ", collected " << hostsLatency(m->getSrcAddr()).
                      getCount() << "mean is now: " << hostsLatency(m->getSrcAddr()).
                      getMean() << endl;
                      WuClatenciesRawCSMA.record(theWuCLatencyCSMA.dbl());
                    } else {
                          EV<< "Received a WuC(CSMA) packet from host[" << m->getSrcAddr()
                          << "], latency=" << theWuCLatencyCSMA << endl;
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
        //world->publishBBItem(catPacket, &packet, hostID);
            if (stats) {
                simtime_t theWuCLatencyPCA = m->getArrivalTime() - m->getCreationTime();
                        if(trace) {
                          hostsLatency(m->getSrcAddr()).collect(theWuCLatencyPCA);
                          WuClatenciesRawPCA.record(theWuCLatencyPCA.dbl());
                        }
                        WuClatencyPCA.collect(theWuCLatencyPCA);
                        if (firstWuCGenerationPCA < 0)
                            firstWuCGenerationPCA = m->getCreationTime();
                        lastWuCReceptionPCA = m->getArrivalTime();
                        if(trace) {
                          EV<< "Received a WuC (PCA) packet from host[" << m->getSrcAddr()
                          << "], latency=" << theWuCLatencyPCA
                          << ", collected " << hostsLatency(m->getSrcAddr()).
                          getCount() << "mean is now: " << hostsLatency(m->getSrcAddr()).
                          getMean() << endl;
                          WuClatenciesRawPCA.record(theWuCLatencyPCA.dbl());
                        } else {
                              EV<< "Received a WuC (PCA) packet from host[" << m->getSrcAddr()
                              << "], latency=" << theWuCLatencyPCA << endl;
                        }
                    }
                    delete msg;

                    //  sendReply(m);
                    break;
    case DATA_MESSAGE:
        m = static_cast<ApplPkt *> (msg);
        nbPacketsReceived++;
        packet.setPacketSent(false);
        packet.setNbPacketsSent(0);
        packet.setNbPacketsReceived(1);
        packet.setHost(myAppAddr);
        //world->publishBBItem(catPacket, &packet, hostID);
        if (stats) {
            simtime_t theLatency = m->getArrivalTime() - m->getCreationTime();
            if(trace) {
              hostsLatency(m->getSrcAddr()).collect(theLatency);
              latenciesRaw.record(theLatency.dbl());
            }
            latency.collect(theLatency);
            if (firstPacketGeneration < 0)
                firstPacketGeneration = m->getCreationTime();
            lastPacketReception = m->getArrivalTime();
            if(trace) {
              EV<< "Received a data packet from host[" << m->getSrcAddr()
              << "], latency=" << theLatency
              << ", collected " << hostsLatency(m->getSrcAddr()).
              getCount() << "mean is now: " << hostsLatency(m->getSrcAddr()).
              getMean() << endl;
              latenciesRaw.record(theLatency.dbl());
            } else {
                  EV<< "Received a data packet from host[" << m->getSrcAddr()
                  << "], latency=" << theLatency << endl;
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
            //world->publishBBItem(catPacket, &packet, hostID);
            if (stats) {
                simtime_t theLatencyDataCSMA = m->getArrivalTime() - m->getCreationTime();
                if(trace) {
                  hostsLatency(m->getSrcAddr()).collect(theLatencyDataCSMA);
                  latenciesRawCSMA.record(theLatencyDataCSMA.dbl());
                }
                latencyDataCSMA.collect(theLatencyDataCSMA);
                if (firstPacketGenerationCSMA < 0)
                    firstPacketGenerationCSMA = m->getCreationTime();
                lastPacketReceptionCSMA = m->getArrivalTime();
                if(trace) {
                  EV<< "Received a data packet from host[" << m->getSrcAddr()
                  << "], latency=" << theLatencyDataCSMA
                  << ", collected " << hostsLatency(m->getSrcAddr()).
                  getCount() << "mean is now: " << hostsLatency(m->getSrcAddr()).
                  getMean() << endl;
                  latenciesRawCSMA.record(theLatencyDataCSMA.dbl());
                } else {
                      EV<< "Received a data packet from host[" << m->getSrcAddr()
                      << "], latency=" << theLatencyDataCSMA << endl;
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
        //world->publishBBItem(catPacket, &packet, hostID);
        if (stats) {
            simtime_t theACKLatency = m->getArrivalTime() - m->getCreationTime();
            if(trace) {
                hostsLatency(m->getSrcAddr()).collect(theACKLatency);
                ACKlatenciesRaw.record(theACKLatency.dbl());
            }
            ACKlatency.collect(theACKLatency);
            if (firstACKGeneration < 0)
                firstACKGeneration = m->getCreationTime();
                lastACKReception = m->getArrivalTime();
                if(trace) {
                    EV<< "Received an ACK packet from host[" << m->getSrcAddr()
                      << "], latency=" << theACKLatency
                      << ", collected " << hostsLatency(m->getSrcAddr()).
                      getCount() << "mean is now: " << hostsLatency(m->getSrcAddr()).
                      getMean() << endl;
                      ACKlatenciesRaw.record(theACKLatency.dbl());
                    } else {
                          EV<< "Received an ACK packet from host[" << m->getSrcAddr()
                          << "], latency=" << theACKLatency << endl;
                    }
                }
                delete msg;

                //  sendReply(m);
                break;
        default:
        EV << "Error! got packet with unknown kind: " << msg->getKind() << endl;
        delete msg;
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
void SensorAppLayerWuRPCA::handleSelfMsg(cMessage * msg) {
    switch (msg->getKind()) {
    case SEND_DATA_TIMER:
        if (myAppAddr != 0){

               sendWuCPCA();



        }
        break;
    default:
        EV<< "Unkown selfmessage! -> delete, kind: " << msg->getKind() << endl;
        delete msg;
    }
}

void SensorAppLayerWuRPCA::handleLowerControl(cMessage * msg) {
    switch (msg->getKind()) {
   case BaseMacLayer::WUC_START_TX:
        if (myAppAddr != 0){
        send (new cMessage("POWER_ON_PCA", IControl::POWER_ON_PCA),lowerControlOut);
        EV <<"WUC_START_TX send to up layer";
        }
        break;
    case IControl::AWAKE_PCA:
        EV<< "Received Control MSG: " << msg->getName() << endl;
        if (myAppAddr != 0){
        sendDataPCA();
        }
        break;
    case IControl::SLEEPING_PCA:
        if (myAppAddr != 0){

          /* if (nbWuCPCA > sentWuCsPCA){
                 sendWuCPCA();

           }
           else */

//******************

             if (nbPackets > sentWuCsCSMA){
                                sendWuCCSMA();
                                    }
             else  if (nbWuCPCA > sentWuCsPCA){
                          sendWuCPCA();
                   }

//***************************


        }
        break;
    case BaseMacLayer::WUC_START_TX_CSMA:
         if (myAppAddr != 0){
         send (new cMessage("POWER_ON_CSMA", IControl::POWER_ON_CSMA),lowerControlOut);
         EV <<"WUC_START_TX_CSMA received and TURN_ON_CSMA sent to down layer";
         }
         break;
    case IControl::AWAKE_CSMA:
            EV<< "Received Control MSG: " << msg->getName() << endl;
            if (myAppAddr != 0){
            sendDataCSMA();
            }

        /*if (myAppAddr != 0){
        send (new cMessage("TURN_OFF", IControllable::TURN_OFF),lowerControlOut);
        EV<< "turned off msg is sent down" << endl;
        }*/
        break;
    default:
            EV<< "Unkown Control message! -> delete, kind: " << msg->getKind() << endl;
            delete msg;

    }
   // delete msg;
}

/**
  * @brief This function creates a new data message and sends it down to
  * the network layer
 **/
void SensorAppLayerWuRPCA::sendWuCCSMA() {
    ApplPkt *pkt = new ApplPkt("WUC_MESSAGE_CSMA", WUC_MESSAGE_CSMA);

    if(broadcastPackets) {
        pkt->setDestAddr(LAddress::L3BROADCAST);
    } else {
        pkt->setDestAddr(destAddr);
    }
    pkt->setSrcAddr(myAppAddr);
    pkt->setByteLength(headerLength);
    // set the control info to tell the network layer the layer 3 address
    pkt->setControlInfo(new NetwControlInfo(pkt->getDestAddr()));
    EV<< "Sending WuC packet!\n";
   // sendDown(pkt);

    // give time for the radio to be in Tx state before transmitting
                sendDelayed(pkt, WuCperiod, lowerLayerOut);
    nbWuCsSentCSMA++;
    packet.setPacketSent(true);
    packet.setNbPacketsSent(1);
    packet.setNbPacketsReceived(0);
    packet.setHost(myAppAddr);
    //world->publishBBItem(catPacket, &packet, hostID);
    sentWuCsCSMA++;

   // scheduleNextPacket();

}

void SensorAppLayerWuRPCA::sendWuCPCA() {
    ApplPkt *pkt = new ApplPkt("WUC_MESSAGE_PCA", WUC_MESSAGE_PCA);

    if(broadcastPackets) {
        pkt->setDestAddr(LAddress::L3BROADCAST);
    } else {
        pkt->setDestAddr(destAddr);
    }
    pkt->setSrcAddr(myAppAddr);
    pkt->setByteLength(headerLength);
    // set the control info to tell the network layer the layer 3 address
    pkt->setControlInfo(new NetwControlInfo(pkt->getDestAddr()));
    EV<< "Sending WuC packet!\n";
    //sendDown(pkt);
    //scheduleAt(simTime() + WuCperiod, delayTimer);
    // give time for the radio to be in Tx state before transmitting
                    sendDelayed(pkt, WuCperiod, lowerLayerOut);
    nbWuCsSentPCA++;
    packet.setPacketSent(true);
    packet.setNbPacketsSent(1);
    packet.setNbPacketsReceived(0);
    packet.setHost(myAppAddr);
    //world->publishBBItem(catPacket, &packet, hostID);
    sentWuCsPCA++;
    //scheduleNextPacket();

}


/**
  * @brief This function creates a new data message and sends it down to
  * the network layer
 **/
void SensorAppLayerWuRPCA::sendDataCSMA() {
    ApplPkt *pkt = new ApplPkt("DataCSMA", DATA_MESSAGE_CSMA);

    if(broadcastPackets) {
        pkt->setDestAddr(LAddress::L3BROADCAST);
    } else {
        pkt->setDestAddr(destAddr);
    }
    pkt->setSrcAddr(myAppAddr);
    //pkt->setByteLength(headerLength);
    pkt->setBitLength(PAYLOAD_SIZE);
    // set the control info to tell the network layer the layer 3 address
    pkt->setControlInfo(new NetwControlInfo(pkt->getDestAddr()));
    EV<< "Sending data packet!\n";
    sendDown(pkt);
    //sendDelayed(pkt, beaconPeriod, lowerGateOut);
    nbPacketsSentCSMA++;
    packet.setPacketSent(true);
    packet.setNbPacketsSent(1);
    packet.setNbPacketsReceived(0);
    packet.setHost(myAppAddr);
    //world->publishBBItem(catPacket, &packet, hostID);
    sentPackets++;
    scheduleNextPacket();
}

void SensorAppLayerWuRPCA::sendDataPCA() {
    ApplPkt *pkt = new ApplPkt("Data", DATA_MESSAGE);

    if(broadcastPackets) {
        pkt->setDestAddr(LAddress::L3BROADCAST);
    } else {
        pkt->setDestAddr(destAddr);
    }
    pkt->setSrcAddr(myAppAddr);
    //pkt->setByteLength(headerLength);
    pkt->setBitLength(PAYLOAD_SIZE);
    // set the control info to tell the network layer the layer 3 address
    pkt->setControlInfo(new NetwControlInfo(pkt->getDestAddr()));
    EV<< "Sending data packet!\n";
    sendDown(pkt);
    //sendDelayed(pkt, beaconPeriod, lowerGateOut);
    nbPacketsSent++;
    packet.setPacketSent(true);
    packet.setNbPacketsSent(1);
    packet.setNbPacketsReceived(0);
    packet.setHost(myAppAddr);
    //world->publishBBItem(catPacket, &packet, hostID);
    sentPacketsPCA++;
    //scheduleNextPacketPCA();
   // sendWuCCSMA();
}

void SensorAppLayerWuRPCA::finish() {
    if (stats) {
        if (trace) {
            // output logs to scalar file
            for (map<int, cStdDev>::iterator it = latencies.begin(); it
                    != latencies.end(); ++it) {
                char dispstring[12];
                cStdDev aLatency = it->second;
                sprintf(dispstring, "latency%d", it->first);
                //dispstring
                recordScalar(dispstring, aLatency.getMean(), "s");
                aLatency.record();
            }

            for (map<int, cStdDev>::iterator it = latenciesCSMA.begin(); it
                                != latenciesCSMA.end(); ++it) {
                            char dispstring[12];
                            cStdDev aLatencyCSMA = it->second;
                            sprintf(dispstring, "latency%d", it->first);
                            //dispstring
                            recordScalar(dispstring, aLatencyCSMA.getMean(), "s");
                            aLatencyCSMA.record();
                        }

           for (map<int, cStdDev>::iterator it = WuClatenciesCSMA.begin(); it
                   != WuClatenciesCSMA.end(); ++it) {
                char WuCdispstring[12];
                cStdDev WuCaLatencyCSMA = it->second;
                                sprintf(WuCdispstring, "latency%d", it->first);
                                //dispstring
                                recordScalar(WuCdispstring, WuCaLatencyCSMA.getMean(), "s");
                                WuCaLatencyCSMA.record();
            }

           for (map<int, cStdDev>::iterator it = WuClatenciesPCA.begin(); it
                              != WuClatenciesPCA.end(); ++it) {
                           char WuCdispstring[12];
                           cStdDev WuCaLatencyPCA = it->second;
                                           sprintf(WuCdispstring, "latency%d", it->first);
                                           //dispstring
                                           recordScalar(WuCdispstring, WuCaLatencyPCA.getMean(), "s");
                                           WuCaLatencyPCA.record();
                       }

           for (map<int, cStdDev>::iterator it = ACKlatencies.begin(); it
                            != ACKlatencies.end(); ++it) {
                        char ACKdispstring[12];
                        cStdDev ACKaLatency = it->second;
                        sprintf(ACKdispstring, "latency%d", it->first);
                        //dispstring
                        recordScalar(ACKdispstring, ACKaLatency.getMean(), "s");
                        ACKaLatency.record();
                    }
        }
        recordScalar("activity duration", lastPacketReception
                - firstPacketGeneration, "s");
        recordScalar("firstPacketGeneration", firstPacketGeneration, "s");
        recordScalar("lastPacketReception", lastPacketReception, "s");
        recordScalar("nbPacketsSent", nbPacketsSent);
        recordScalar("nbPacketsReceived", nbPacketsReceived);
        latency.record();


        recordScalar("activity duration", lastPacketReceptionCSMA
                       - firstPacketGenerationCSMA, "s");
               recordScalar("firstPacketGenerationCSMA", firstPacketGenerationCSMA, "s");
               recordScalar("lastPacketReceptionCSMA", lastPacketReceptionCSMA, "s");
               recordScalar("nbPacketsSentCSMA", nbPacketsSentCSMA);
               recordScalar("nbPacketsSentCSMA", nbPacketsSentCSMA);
               recordScalar("nbPacketsReceivedCSMA", nbPacketsReceivedCSMA);
               latencyDataCSMA.record();

       recordScalar("WuC activity duration", lastWuCReceptionCSMA
                        - firstWuCGenerationCSMA, "s");
       recordScalar("firstWuCGenerationCSMA", firstWuCGenerationCSMA, "s");
                recordScalar("lastWuCReceptionCSMA", lastWuCReceptionCSMA, "s");
                recordScalar("nbWuCsSentCSMA", nbWuCsSentCSMA);
                recordScalar("nbWuCsReceivedCSMA", nbWuCsReceivedCSMA);
                WuClatencyCSMA.record();

       recordScalar("WuC activity duration", lastWuCReceptionPCA
                                        - firstWuCGenerationPCA, "s");
       recordScalar("firstWuCGenerationPCA", firstWuCGenerationPCA, "s");
       recordScalar("lastWuCReceptionPCA", lastWuCReceptionPCA, "s");
       recordScalar("nbWuCsSentPCA", nbWuCsSentPCA);
       recordScalar("nbWuCsReceivedPCA", nbWuCsReceivedPCA);
       WuClatencyPCA.record();

        recordScalar("ACK activity duration", lastACKReception
                                        - firstACKGeneration, "s");
                       recordScalar("firstACKGeneration", firstACKGeneration, "s");
                                recordScalar("lastACKReception", lastACKReception, "s");
                                //recordScalar("nbWuCsSent", nbWuCsSent);
                                recordScalar("nbACKsReceived", nbACKsReceived);
                                ACKlatency.record();
    }
    cComponent::finish();
}

SensorAppLayerWuRPCA::~SensorAppLayerWuRPCA() {
    cancelAndDelete(delayTimer);
    cancelAndDelete(beaconTimer);
}

