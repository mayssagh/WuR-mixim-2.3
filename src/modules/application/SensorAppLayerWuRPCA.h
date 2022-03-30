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

#ifndef __MIXIM_SENSORAPPLAYERWURPCA_H_
#define __MIXIM_SENSORAPPLAYERWURPCA_H_

#include <omnetpp.h>
#include <map>
//#include "BaseUtility.h"
#include "BaseModule.h"
#include "BaseLayer.h"
#include "BaseLayerDual.h"
#include "NetwControlInfo.h"
#include "Packet.h"
#include "ApplPkt_m.h"
#include "BaseApplLayer.h"
#include "BaseWorldUtility.h"

using namespace std;

/**
 * TODO - Generated class
 */
class MIXIM_API SensorAppLayerWuRPCA : public  BaseLayer
{
  public:


  /** @brief Initialization of the module and some variables*/
  virtual void initialize(int);
  virtual void finish();

  ~SensorAppLayerWuRPCA();

  SensorAppLayerWuRPCA(): packet(100) {} // we must specify a packet length for Packet.h

  enum APPL_MSG_TYPES
  {
    SEND_DATA_TIMER ,
    WUC_MESSAGE_PCA = 1,
    DATA_MESSAGE = 2,
    WUC_MESSAGE_CSMA = 3,
    DATA_MESSAGE_CSMA = 4,
    DATA=5,
    ACK=9,
    BEACON_TIMER,
    STOP_SIM_TIMER,

  };

  enum TRAFFIC_TYPES
  {
    UNKNOWN=0,
    PERIODIC,
    UNIFORM,
    EXPONENTIAL,
    NB_DISTRIBUTIONS,
  };

protected:
  cMessage * delayTimer;
  cMessage * beaconTimer;
  int myAppAddr;
  int destAddr;
  int sentPackets;
  int sentPacketsPCA;
  int sentWuCsCSMA;
  int sentWuCsPCA;
  simtime_t initializationTime;
  simtime_t firstWuCGenerationCSMA;
  simtime_t lastWuCReceptionCSMA;
  simtime_t firstWuCGenerationPCA;
  simtime_t lastWuCReceptionPCA;
  simtime_t firstPacketGeneration;
  simtime_t lastPacketReception;
  simtime_t firstPacketGenerationCSMA;
  simtime_t lastPacketReceptionCSMA;
  simtime_t firstACKGeneration;
  simtime_t lastACKReception;
  // parameters:
  int trafficType;
  double trafficParam;
  double WuCperiod;
  int nbPackets;
  int nbWuCPCA;
  long nbPacketsSent;
  long nbPacketsSentCSMA;
  long nbWuCsSentCSMA;
  long nbWuCsSentPCA;
  long nbPacketsReceived;
  long nbPacketsReceivedCSMA;
  long nbWuCsReceivedCSMA;
  long nbWuCsReceivedPCA;
  long nbACKsReceived;
  bool stats;
  bool trace;
  bool debug;
  bool broadcastPackets;
  map < int, cStdDev > latencies;
  map < int, cStdDev > latenciesCSMA;
  map < int, cStdDev > WuClatenciesCSMA;
  map < int, cStdDev > WuClatenciesPCA;
  map < int, cStdDev > ACKlatencies;
  cStdDev latency;
  cStdDev latencyDataCSMA;
  cStdDev WuClatencyCSMA;
  cStdDev WuClatencyPCA;
  cStdDev ACKlatency;
  cOutVector latenciesRaw;
  cOutVector latenciesRawCSMA;
  cOutVector WuClatenciesRawCSMA;
  cOutVector WuClatenciesRawPCA;
  cOutVector ACKlatenciesRaw;
  Packet packet; // informs the simulation of the number of packets sent and received by this node.
  int catPacket;
  int hostID;
  int headerLength;
  int PAYLOAD_SIZE;
  BaseWorldUtility* world;
  double beaconPeriod;

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

  virtual void handleUpperMsg(cMessage * m) { delete m; }

  virtual void handleUpperControl(cMessage * m) { delete m; }

  virtual void handlelowerWuC(cMessage* msg) {
             error("BaseNetwLayer does not handle control messages");
         }

         virtual void handleUpperMsgWuC(cMessage* msg) {
             error("BaseNetwLayer does not handle control messages");
         }

         virtual void handleLowerControlWuC(cMessage* msg) {
             error("BaseNetwLayer does not handle control messages");
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
  virtual void scheduleNextPacketPCA();

  /**
   * @brief Returns the latency statistics variable for the passed host address.
   * @param hostAddress the address of the host to return the statistics for.
   * @return A reference to the hosts latency statistics.
   */
  cStdDev& hostsLatency(int hostAddress);
};




#endif
