/*
 * BaseLayerDual.cc

 *
 *  Created on: 19 août 2020
 *      Author: asus
 */

#include "BaseLayerDual.h"
#include <cassert>
const simsignalwrap_t BaseLayerDual::catPassedMsgSignal     = simsignalwrap_t(MIXIM_SIGNAL_PASSEDMSG_NAME);
const simsignalwrap_t BaseLayerDual::catPacketSignal        = simsignalwrap_t(MIXIM_SIGNAL_PACKET_NAME);
const simsignalwrap_t BaseLayerDual::catDroppedPacketSignal = simsignalwrap_t(MIXIM_SIGNAL_DROPPEDPACKET_NAME);

void BaseLayerDual::initialize(int stage)
{
    BaseModule::initialize(stage);
    if(stage == 0) {
                //upperLayerIn_dual  = findGate("upperLayerIn_dual");
                upperLayerIn_dual  = findGate("upperLayerIn_dual");
                upperLayerOut_dual = findGate("upperLayerOut_dual");
                lowerLayerIn_dual  = findGate("lowerLayerIn_dual");
                lowerLayerOut_dual = findGate("lowerLayerOut_dual");
                upperControlIn_dual = findGate("upperControlIn_dual");
                upperControlOut_dual = findGate("upperControlOut_dual");
                lowerControlIn_dual  = findGate("lowerControlIn_dual");
                lowerControlOut_dual = findGate("lowerControlOut_dual");

                upperLayerInWuR_dual = findGate("upperLayerInWuR_dual");
                upperLayerOutWuR_dual = findGate("upperLayerOutWuR_dual");
                lowerLayerOutWuR_dual = findGate("lowerLayerOutWuR_dual");
                upperControlInWuR_dual = findGate("upperControlInWuR_dual");
                lowerLayerInWuR_dual = findGate("lowerLayerInWuR_dual");
                upperControlOutWuR_dual = findGate("upperControlOutWuR_dual");
                lowerControlInWuR_dual = findGate("lowerControlInWuR_dual");
                lowerControlOutWuR_dual = findGate("lowerControlOutWuR_dual");

                catPassedMsgSignal.initialize();
                catPacketSignal.initialize();
                catDroppedPacketSignal.initialize();

                headerLength = par("headerLength");
                // Find the IDs of the nics.
                cModule* host = findHost();
                if(host == 0) {
                     throw cRuntimeError("Host pointer is NULL in BaseLayerDual");
                 }
                 cModule* MainNic = host->getSubmodule("MainNic");
                 if(MainNic == 0) {
                     throw cRuntimeError("Could not find module 'MainNic' in BaseLayerDual");
                  }
                 cModule* nicController = MainNic->getSubmodule("nicController");
                  if (nicController != 0) {
                      // primaryController = check_and_cast<nicController*>(nicController);
                       if(primaryController == 0) {
                           throw cRuntimeError("Could not find module primary 'nicController' in BaseLayerDual");
                       }
                       primaryNicId = primaryController->getNicId();
                  } else {
                       primaryController = 0;
                       primaryNicId = -1;
                  }

                        cModule* WuRNic = host->getSubmodule("WuRNic");
                        if(WuRNic == 0) {
                            throw cRuntimeError("Could not find module 'MainNic' in BaseLayerDual");
                        }


    }
}

  void BaseLayerDual::sendDown(cMessage *msg)
  {
      send(msg, lowerLayerOut_dual );
  }

  void BaseLayerDual::sendDownWuC(cMessage *msg)
    {
        send(msg, lowerLayerOutWuR_dual );
    }


  void BaseLayerDual::sendUp(cMessage *msg)
    {
        send(msg, upperLayerOut_dual );
    }

  void BaseLayerDual::sendUpWuC(cMessage *msg)
      {
          send(msg, upperLayerOutWuR_dual );
      }

  void BaseLayerDual::sendControlUp(cMessage *msg)
      {
          send(msg, upperControlOut_dual );
      }
  void BaseLayerDual::sendControlUpWuC(cMessage *msg)
        {
            send(msg, upperControlOutWuR_dual );
        }
  void BaseLayerDual::sendControlDown(cMessage *msg)
        {
            send(msg, lowerControlOut_dual );
        }
  void BaseLayerDual::sendControlDownWuC(cMessage *msg)
          {
              send(msg, lowerControlOutWuR_dual );
          }


  void BaseLayerDual::handleMessage(cMessage* msg)
  {
      if (msg->isSelfMessage() ){
          handleSelfMsg(msg);
      } else if(msg->getArrivalGateId() == upperLayerIn_dual) {
          handleUpperMsg(msg);
      } else if(msg->getArrivalGateId() == lowerLayerInWuR_dual) {
          handlelowerWuC(msg);
      } else if(msg->getArrivalGateId() == lowerControlIn_dual ) {
          handleLowerControl(msg);
      } else if(msg->getArrivalGateId() == upperControlIn_dual ) {
          handleUpperControl(msg);
      }else if(msg->getArrivalGateId() == lowerLayerIn_dual ) {
          handleLowerMsg(msg);}
        else if (msg->getArrivalGateId() == lowerControlInWuR_dual){
            handleLowerControlWuC(msg);

      } else if (msg->getArrivalGateId() == upperLayerInWuR_dual){
          handleUpperMsgWuC(msg);

    }else {
          throw cRuntimeError("Received message on unknown gate %d", msg->getArrivalGateId());
      }
  }










