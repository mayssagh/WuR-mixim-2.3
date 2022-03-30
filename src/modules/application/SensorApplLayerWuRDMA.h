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

#ifndef __MIXIM_SENSORAPPLLAYERWURDMA_H_
#define __MIXIM_SENSORAPPLLAYERWURDMA_H_

#include <omnetpp.h>
#include <map>

#include "MiXiMDefs.h"
#include "BaseModule.h"
#include "BaseLayer.h"
#include "Packet.h"
#include "SimpleAddress.h"
using namespace std;

class BaseWorldUtility;

/**
 * TODO - Generated class
 */
class MIXIM_API SensorApplLayerWuRDMA : public BaseLayer
{
private:
        /** @brief Copy constructor is not allowed.
         */
    SensorApplLayerWuRDMA(const SensorApplLayerWuRDMA&);
        /** @brief Assignment operator is not allowed.
         */
    SensorApplLayerWuRDMA& operator=(const SensorApplLayerWuRDMA&);

    public:
        /** @brief Initialization of the module and some variables*/
        virtual void initialize(int);
        virtual void finish();

        virtual ~SensorApplLayerWuRDMA();

        SensorApplLayerWuRDMA() :
                BaseLayer(), delayTimer(NULL),delayTimerCSMA(NULL), myAppAddr(), destAddr(), sentPacketsCSMA(0),sentPacketsPCA(0),sentWuCCSMA(0), sentWuCPCA(0), initializationTime(), firstPacketGenerationCSMA(), lastPacketReceptionCSMA(), firstPacketGenerationPCA(), lastPacketReceptionPCA(),
                firstWuCGenerationCSMA(), lastWuCReceptionCSMA(), firstWuCGenerationPCA(), lastWuCReceptionPCA(), firstACKGeneration(), lastACKReception(), WuCTime(), TimeON(), trafficType(0), trafficParam(0.0), nbPacketsCSMA(0),nbPacketsPCA(0), nbPacketsSentCSMA(0), nbPacketsReceivedCSMA(0), nbPacketsSentPCA(0), nbPacketsReceivedPCA(0), nbWuCsSentCSMA(0), nbWuCsReceivedCSMA(0), nbWuCsSentPCA(0), nbWuCsReceivedPCA(0), nbACKsReceived(0), stats(false), trace(
                        false), debug(false), broadcastPackets(false), latenciesCSMA(), latencyCSMA(), latenciesRawCSMA(), latenciesPCA(), latencyPCA(), latenciesRawPCA(),latenciesWuCCSMA(), latencyWuCCSMA(), latenciesRawWuCCSMA(), latenciesWuCPCA(), latencyWuCPCA(), latenciesRawWuCPCA(), latenciesACK(), latencyACK(), latenciesRawACK(),packet(
                        100), headerLength(0), packetLength(0), WuCLength(0), world(NULL), dataOut(0), dataIn(0), ctrlOut(0), ctrlIn(0)
        {
        } // we must specify a packet length for Packet.h

        enum APPL_MSG_TYPES
        {
            SEND_DATA_TIMER,
            WUC_MESSAGE_PCA = 1,
            DATA_MESSAGE = 2,
            WUC_MESSAGE_CSMA = 3,
            DATA_MESSAGE_CSMA = 4,
            DATA=5,
            ACK=9,
            ACK_CSMA = 17,
            SEND_DATA_TIMER_CSMA,
            SEND_DATA_TIMER_PCA,
            STOP_SIM_TIMER,
        };

        enum TRAFFIC_TYPES
        {
            UNKNOWN = 0, PERIODIC, UNIFORM, EXPONENTIAL, NB_DISTRIBUTIONS,
        };

    protected:
        cMessage * delayTimer;
        cMessage * delayTimerCSMA;
        cMessage * delayTimerPCA;
        LAddress::L3Type myAppAddr;
        LAddress::L3Type destAddr;
        int sentPacketsCSMA;
        int sentPacketsPCA;
        int sentWuCCSMA;
        int sentWuCPCA;
        simtime_t initializationTime;
        simtime_t firstPacketGenerationCSMA;
        simtime_t lastPacketReceptionCSMA;

        simtime_t firstPacketGenerationPCA;
        simtime_t lastPacketReceptionPCA;

        simtime_t firstWuCGenerationCSMA;
        simtime_t lastWuCReceptionCSMA;

        simtime_t firstWuCGenerationPCA;
        simtime_t lastWuCReceptionPCA;

        simtime_t firstACKGeneration;
        simtime_t lastACKReception;
        simtime_t WuCTime;
        simtime_t TimeON;

        // parameters:
        int trafficType;
        double trafficParam;
        int nbPacketsCSMA;
        int nbPacketsPCA;
        long nbPacketsSentCSMA;
        long nbPacketsReceivedCSMA;
        long nbPacketsSentPCA;
        long nbPacketsReceivedPCA;
        long nbWuCsSentCSMA;
        long nbWuCsReceivedCSMA;
        long nbWuCsSentPCA;
        long nbWuCsReceivedPCA;
        long nbACKsReceived;
        bool stats;
        bool trace;
        bool debug;
        bool broadcastPackets;
        std::map<LAddress::L3Type, cStdDev> latenciesCSMA;
        cStdDev latencyCSMA;
        cOutVector latenciesRawCSMA;

        std::map<LAddress::L3Type, cStdDev> latenciesPCA;
        cStdDev latencyPCA;
        cOutVector latenciesRawPCA;

        std::map<LAddress::L3Type, cStdDev> latenciesWuCCSMA;
        cStdDev latencyWuCCSMA;
        cOutVector latenciesRawWuCCSMA;

        std::map<LAddress::L3Type, cStdDev> latenciesWuCPCA;
        cStdDev latencyWuCPCA;
        cOutVector latenciesRawWuCPCA;

        std::map<LAddress::L3Type, cStdDev> latenciesACK;
        cStdDev latencyACK;
        cOutVector latenciesRawACK;

        Packet packet; // informs the simulation of the number of packets sent and received by this node.
        int headerLength;
        int packetLength;
        int WuCLength;
        BaseWorldUtility* world;

    protected:
        // gates
        int dataOut;
        int dataIn;
        int ctrlOut;
        int ctrlIn;

        /** @brief Handle self messages such as timer... */
        virtual void handleSelfMsg(cMessage *);

        /** @brief Handle messages from lower layer */
        virtual void handleLowerMsg(cMessage *);

        virtual void handleLowerControl(cMessage * msg);

        virtual void handleUpperMsg(cMessage * m)
        {
            delete m;
        }

        virtual void handleUpperControl(cMessage * m)
        {
            delete m;
        }

        /** @brief send a data packet to the next hop */
        virtual void sendDataCSMA();
        virtual void sendDataPCA();

        virtual void sendWuCCSMA();

        virtual void sendWuCPCA();

        /** @brief Recognize distribution name. Redefine this method to add your own distribution. */
        virtual void initializeDistribution(const char*);

        /** @brief calculate time to wait before sending next packet, if required. You can redefine this method in a subclass to add your own distribution. */
        virtual void scheduleNextPacket();
        virtual void scheduleNextPacketCSMA();

        /**
         * @brief Returns the latency statistics variable for the passed host address.
         * @param hostAddress the address of the host to return the statistics for.
         * @return A reference to the hosts latency statistics.
         */
        cStdDev& hostsLatency(const LAddress::L3Type& hostAddress);
};

#endif
