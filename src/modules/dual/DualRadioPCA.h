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

#ifndef __MIXIM_DUALRADIOPCA_H_
#define __MIXIM_DUALRADIOPCA_H_

#include <omnetpp.h>
#include "BaseLayerDual.h"
#include <omnetpp.h>
#include "MacToPhyInterface.h"
#include "BaseLayer.h"
#include <NetwPkt_m.h>
using namespace std;

/**
 * TODO - Generated class
 */
class DualRadioPCA : public BaseLayerDual
{
  public:

  /** @brief frame kinds */
  enum DualRadioMessageKinds {

      INIT_MSG = 0,
      WUC_MESSAGE_PCA =1 ,
      DATA_MESSAGE = 2,
      WUC_MESSAGE_CSMA = 3,
      TX_OVER = 4,
      RADIO_SWITCHED = 5,
      WUC_START_TX = 6,
      //MAC_ERROR,
      PACKET_DROPPED = 7,
      RX_TIMEOUT_MSG_TYPE = 8,
      ACK = 9,
      ACK_SENT =10,
     //WUC_MESSAGE_PCA =11 ,
     TRANSMISSION_OVER = 12,
     INIT_MSG_CSMA,
     TX_OVER_CSMA,
     PACKET_DROPPED_CSMA,
     RX_TIMEOUT_MSG_CSMA , //oui
     ACK_CSMA = 17 ,
     ACK_SENT_CSMA,
     TRANSMISSION_OVER_CSMA,
     RADIO_SWITCHED_CSMA = 20,

  };



  enum DualRadioStates
          {
          INIT_STATE //= 0
          ,SWITCHING_RADIO_TO_SLEEP_STATE //= 1
          ,SLEEP_STATE //= 2
          , WuC_TO_DATA_TX_STATE //= 3
          ,TX_DATA_STATE //= 4
          , RX_DATA_STATE //= 5
          ,TX_ACK_STATE
          };
          DualRadioStates Radio_STATE;

protected:
  /** @brief Handler to the physical layer.*/
  MacToPhyInterface* dual_to_phy;
  /** @brief Time to switch radio  to Tx state */
  simtime_t TIME_TO_TXSETUP;
  bool isSink;
  /** @name Pointer for timer messages.*/
      /*@{*/
  cMessage * rx_timeout_msg;
  cMessage * init_msg;
  cMessage * init_msg_csma;
  /** @brief Time to setup radio from sleep to Rx state */
  simtime_t RX_TIMEOUT;
  simtime_t TIME_TO_GO_TO_SLEEP;

   /** @brief Do we track statistics?*/
  bool doStats;
  /** @brief Blackboard category for PassedMessage BBItems.*/
  int  catPassedMsg;
  /** @brief The last message passed through this layer.*/
  PassedMessage *passedMsg;

  long nbTxFrames;
  long nbRxFrames;
  long nbTxWuC;
  long nbRxWuC;
  long nbDroppedFrames;
  long nbDroppedWuC;
  bool stats;

  /** @brief The hosts id. */
  int hostId;



public:
  virtual void initialize(int);
  virtual void finish();
  virtual ~DualRadioPCA();

protected:
  virtual void handleSelfMsg(cMessage* msg);
  virtual void handleUpperMsg(cMessage* msg);
  virtual void handlelowerWuC(cMessage* msg);
  virtual void handleLowerControl(cMessage* msg);
  virtual void handleUpperControl(cMessage* msg);
  virtual void handleLowerMsg(cMessage* msg);
  virtual void handleUpperMsgWuC(cMessage* msg);
          //virtual void handleLowerMsgWuR(cMessage* msg) = 0;
  virtual void handleLowerControlWuC(cMessage* msg);

  void recordPacket(PassedMessage::direction_t dir,
                            PassedMessage::gates_t gate,
                            const cMessage *m);


};


#endif
