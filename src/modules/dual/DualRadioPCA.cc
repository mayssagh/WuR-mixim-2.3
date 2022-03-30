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

#include "DualRadioPCA.h"
#include "BaseLayer.h"
#include "FWMath.h"
#include "IControl.h"
#include "BaseMacLayer.h"
#include "csmaMain2.h"
#include "MacToPhyInterface.h"
#include <SimpleAddress.h>
#include <csimplemodule.h>
#include "FindModule.h"
#include "SensorAppLayerWuRPCA.h"

Define_Module(DualRadioPCA);

void DualRadioPCA::initialize(int stage)
{
    BaseLayerDual::initialize(stage);
    if(stage == 0) {
        dual_to_phy = FindModule<MacToPhyInterface*>::findSubModule(getParentModule());

        if (hasPar("stats") && par("stats").boolValue()) {
            doStats = true;
            passedMsg = new PassedMessage();
            //catPassedMsg = utility->getCategory(passedMsg);
            passedMsg->fromModule = getId();
            hostId = findHost()->getId();
            nbTxWuC = 0;
            nbTxFrames = 0;
            nbRxWuC = 0;
            nbRxFrames = 0;
            nbDroppedFrames = 0;
            nbDroppedWuC = 0;
            stats = par("stats").boolValue();
        }
        else {
            doStats = false;
        }
        Radio_STATE = SLEEP_STATE;
        //Radio_STATE =INIT_STATE;

        TIME_TO_TXSETUP = par("TIME_TO_TXSETUP").doubleValue();
        RX_TIMEOUT = par("RX_TIMEOUT").doubleValue();
        TIME_TO_GO_TO_SLEEP = par("TIME_TO_GO_TO_SLEEP").doubleValue();
        isSink = par("isSink").boolValue();
        // initialize the timers
        rx_timeout_msg = new cMessage("rx_timeout_msg");
        init_msg = new cMessage("init_msg", INIT_MSG);
        init_msg_csma= new cMessage("init_msg_csma", INIT_MSG_CSMA);
    }
}
    /**
     * handle timers
     */
void DualRadioPCA::handleSelfMsg(cMessage* msg){


     EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";

            switch (Radio_STATE) {

            //***************************Init state*************************************
            case INIT_STATE :

                 if (msg->getKind() == INIT_MSG) {
                     EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                    sendControlDown(new cMessage("POWER_OFF_PCA", IControl::POWER_OFF_PCA));
                    //dual_to_phy->setRadioState(Radio::SLEEP);
                    EV << "Radio is powered offO \n";
                    Radio_STATE = SWITCHING_RADIO_TO_SLEEP_STATE ;
                }

                 else if (msg->getKind() == INIT_MSG_CSMA) {
                     EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                     sendControlDown(new cMessage("POWER_OFF_CSMA", IControl::POWER_OFF_CSMA));
                     //dual_to_phy->setRadioState(Radio::SLEEP);
                      EV << "Radio is powered off\n";
                      Radio_STATE = SWITCHING_RADIO_TO_SLEEP_STATE ;
                  }
                else if (msg->getKind() == DATA_MESSAGE || msg->getKind() == TX_OVER || (msg->getKind() == SensorAppLayerWuRPCA::DATA_MESSAGE_CSMA)){
                    EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                    delete msg ;
                }

                break ;
            //*********************SWITCHING_RADIO_TO_SLEEP_STATE******************
            case SWITCHING_RADIO_TO_SLEEP_STATE:
                if ((msg->getArrivalGateId() == lowerControlIn_dual) && ( msg->getKind() == RADIO_SWITCHED)){

                    dual_to_phy = FindModule<MacToPhyInterface*>::findSubModule(getParentModule());
                    if( dual_to_phy->getRadioState() == MiximRadio::SLEEP){
                    EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                    EV << "case SWITCHING_RADIO_TO_SLEEP_STATE" << "\n";
                    Radio_STATE = SLEEP_STATE ;
                    }
                   // delete msg;
                    }

                 else if ((msg->getArrivalGateId() == lowerControlIn_dual) && ( msg->getKind() == RADIO_SWITCHED_CSMA)){

                    dual_to_phy = FindModule<MacToPhyInterface*>::findSubModule(getParentModule());
                    if( dual_to_phy->getRadioState() == MiximRadio::SLEEP){
                    EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                    EV << "case SWITCHING_RADIO_TO_SLEEP_STATE" << "\n";
                    Radio_STATE = SLEEP_STATE ;
                    EV <<"Radio_STATE="<< Radio_STATE << "\n";

                  }
                  //  delete msg;
                   EV <<"Radio_STATE="<< Radio_STATE << "\n";
                }
                break ;
                // *******************************SLEEP_STATE (send or receive WuC)*********************************
            case SLEEP_STATE :
               if((msg->getArrivalGateId() == upperLayerIn_dual) && (msg->getKind() == WUC_MESSAGE_PCA || msg->getKind() == SensorAppLayerWuRPCA::WUC_MESSAGE_CSMA )){
                    EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                    sendDelayed(msg, TIME_TO_TXSETUP, lowerLayerOutWuR_dual);
                    nbTxWuC++;
                    dual_to_phy->setRadioState(MiximRadio::RX);
                    EV <<"Radio_Setup_Rx_ON";
                    Radio_STATE = WuC_TO_DATA_TX_STATE;
                    }
                else if ((msg->getArrivalGateId()== lowerControlIn_dual) && (msg->getKind() == TX_OVER)){
                    EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                    EV <<"RADIO_OFF";
                    delete msg;
                }
                else if ((msg->getArrivalGateId()== lowerLayerInWuR_dual) && (msg->getKind() == WUC_MESSAGE_PCA) && isSink==true){
                    EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                    EV << " Case Receiving APP_WUC_MESSAGE";
                    NetwPkt* incoming_wuc = static_cast<NetwPkt*>(msg ) ;
                    EV <<"incoming wuc source address=" << incoming_wuc->getDestAddr() << endl;
                    if (incoming_wuc->getDestAddr() == findHost()->getIndex()) {
                        //main NIC

                        send (new cMessage("POWER_ON_PCA", IControl::POWER_ON_PCA),lowerControlOut_dual);
                        EV <<"Main Radio OF THE SINK NODE IS TURNED ON";
                        sendUp(incoming_wuc);
                        dual_to_phy->setRadioState(MiximRadio::RX);
                        EV <<"Radio_Setup_Rx_ON";
                        cancelEvent(rx_timeout_msg);
                        scheduleAt(simTime() + RX_TIMEOUT, rx_timeout_msg);
                        Radio_STATE = RX_DATA_STATE;
                        //nbRxWuC++;
                        //delete msg;
                    }
                }

                else if ((msg->getArrivalGateId()== lowerLayerInWuR_dual) && (msg->getKind() == SensorAppLayerWuRPCA::WUC_MESSAGE_CSMA) && isSink==true){
                    EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                       EV << " Case Receiving APP_WUC_MESSAGE_CSMA";
                       NetwPkt* incoming_wuc = static_cast<NetwPkt*>(msg ) ;
                       EV <<"incoming wuc source address=" << incoming_wuc->getDestAddr() << endl;
                       if (incoming_wuc->getDestAddr() == findHost()->getIndex()) {
                        //main NIC
                           send (new cMessage("POWER_ON_CSMA", IControl::POWER_ON_CSMA),lowerControlOut_dual);
                           EV <<"Main Radio OF THE SINK NODE IS TURNED ON";
                           sendUp(incoming_wuc);
                           dual_to_phy->setRadioState(MiximRadio::RX);
                           EV <<"Radio_Setup_Rx_ON";
                           cancelEvent(rx_timeout_msg);
                           scheduleAt(simTime() + RX_TIMEOUT, rx_timeout_msg);
                           Radio_STATE = RX_DATA_STATE;
                           //nbRxWuC++;

                   }
               }
                // WuR receiving DATA_MESSAGES
                else if(msg->getArrivalGateId()== lowerLayerInWuR_dual && ((msg->getKind() == DATA_MESSAGE) || (msg->getKind() == SensorAppLayerWuRPCA::DATA_MESSAGE_CSMA))){
                    EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                    EV << "Case APP_DATA_MESSAGE";
                    delete msg;
                }
                else if (msg->getKind() == WUC_MESSAGE_PCA || msg->getKind() == SensorAppLayerWuRPCA::WUC_MESSAGE_CSMA ){
                    EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                    EV << "msg->getKind() == APP_WUC_MESSAGE fonctionne" ;
                    delete msg;
                }
                else if (msg->getArrivalGateId() == upperLayerIn_dual){
                    EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                    EV << "msg->getArrivalGateId() is OK" ;
                    delete msg;
                }
                break ;
                //************************************WuC_TO_DATA_TX_STATE*****************************************************
            case WuC_TO_DATA_TX_STATE:
                if ((msg->getArrivalGateId()== lowerControlIn_dual) && (msg->getKind() == TX_OVER)){
                    EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";

                    EV <<"RADIO_OFF";
                    delete msg;
                }
                else if (msg->getKind() == BaseMacLayer::RX_TIMEOUT_MSG_TYPE){
                    EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                    Radio_STATE = INIT_STATE;
                    scheduleAt(simTime()+ TIME_TO_GO_TO_SLEEP, init_msg);
                }
                else if (msg->getKind() == BaseMacLayer::RX_TIMEOUT_MSG_TYPE_CSMA){
                    EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                    Radio_STATE = INIT_STATE;
                    scheduleAt(simTime()+ TIME_TO_GO_TO_SLEEP, init_msg_csma);
                }
                else if ((msg->getArrivalGateId() == upperLayerIn_dual) && (msg->getKind() == DATA_MESSAGE)){
                    EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                    EV << "Transmitting DATA" << " in " << Radio_STATE << "\n";
                    dual_to_phy->setRadioState(MiximRadio::TX);
                    EV <<"RADIO_TX_ON \n";
                    //cancelEvent(rx_timeout_msg);
                    dual_to_phy = FindModule<MacToPhyInterface*>::findSubModule(this->getParentModule());
                    NetwPkt* incoming_wuc = static_cast<NetwPkt*>(msg ) ;
                    EV <<"incoming wuc des address=" << incoming_wuc->getDestAddr() << endl;
                    EV <<"incoming wuc source address=" << incoming_wuc->getSrcAddr() << endl;
                    // give time for the radio to be in Tx state before transmitting
                    sendDelayed(msg, TIME_TO_TXSETUP, lowerLayerOut_dual);
                    Radio_STATE = TX_DATA_STATE ;
                    EV <<"from send DATA to TX_DATA_STATE\n";
                    //nbTxFrames++;
                }

                else if ((msg->getArrivalGateId() == upperLayerIn_dual) && (msg->getKind() == SensorAppLayerWuRPCA::DATA_MESSAGE_CSMA)){
                    EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                     EV << "Transmitting DATA" << " in " <<  Radio_STATE << "\n";
                     dual_to_phy->setRadioState(MiximRadio::TX);
                     EV <<"RADIO_TX_ON\n";
                     dual_to_phy = FindModule<MacToPhyInterface*>::findSubModule(this->getParentModule());
                     NetwPkt* incoming_wuc = static_cast<NetwPkt*>(msg ) ;
                     EV <<"incoming WuC destination address= " << incoming_wuc->getDestAddr() << endl;
                     EV <<"incoming WuC source address=" << incoming_wuc->getSrcAddr() << endl;
                     // give time for the radio to be in Tx state before transmitting
                     sendDelayed(msg, TIME_TO_TXSETUP, lowerLayerOut_dual);
                     Radio_STATE = TX_DATA_STATE ;
                     EV <<"from send DATA to TX_DATA_STATE\n";
                     //nbTxFrames++;
                                }
                else if(msg->getKind() == BaseMacLayer::WUC_START_TX){
                    send(msg, upperControlOut_dual);
                    EV <<"WUC_START_TX send to up layer";
                }
                else if(msg->getKind() == BaseMacLayer::WUC_START_TX_CSMA){
                     send(msg, upperControlOut_dual);
                     EV <<"WUC_START_TX_CSMA send to up layer";
                                }
                else if (msg->getKind () == RADIO_SWITCHED){
                    delete msg;
                }
                else if (msg->getArrivalGateId()== lowerControlIn_dual && msg->getKind() == BaseMacLayer::PACKET_DROPPED){
                    EV <<"PACKET_DROPPED";
                    Radio_STATE=INIT_STATE ;
                    scheduleAt(simTime() + TIME_TO_GO_TO_SLEEP, init_msg);
                    delete msg;
                    nbDroppedWuC++;
                }
                else if (msg->getArrivalGateId()== lowerControlIn_dual && msg->getKind() == BaseMacLayer::PACKET_DROPPED_CSMA){
                    EV <<"PACKET_DROPPED";
                    Radio_STATE=INIT_STATE ;
                    scheduleAt(simTime() + TIME_TO_GO_TO_SLEEP, init_msg_csma);
                    delete msg;
                    nbDroppedWuC++;
                }
                break ;
                // ************************TX_DATA_STATE*****************************
            case TX_DATA_STATE:
                 if ((msg->getArrivalGateId()== lowerControlIn_dual) && (msg->getKind() == TRANSMISSION_OVER)){
                    EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                    EV <<"MAC_SUCCESS";
                    Radio_STATE = INIT_STATE;
                    scheduleAt(simTime () + TIME_TO_GO_TO_SLEEP, init_msg);
                    delete msg ;
                }

                 if ((msg->getArrivalGateId()== lowerControlIn_dual) && (msg->getKind() == TRANSMISSION_OVER_CSMA)){
                   EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                   EV <<"MAC_SUCCESS";
                   Radio_STATE = INIT_STATE;
                   scheduleAt(simTime () + TIME_TO_GO_TO_SLEEP, init_msg_csma);
                   delete msg ;
                 }

                else if (msg->getKind() == RADIO_SWITCHED){
                    delete msg ;
                }
                else if (msg->getKind() == BaseMacLayer::PACKET_DROPPED){
                    EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                    Radio_STATE = INIT_STATE;
                    //Reset
                    scheduleAt(simTime () + TIME_TO_GO_TO_SLEEP, init_msg);
                    delete msg ;
                    nbDroppedFrames++;
                }

                else if (msg->getKind() == BaseMacLayer::PACKET_DROPPED_CSMA){
                    EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                    Radio_STATE = INIT_STATE;
                    //Reset
                    scheduleAt(simTime () + TIME_TO_GO_TO_SLEEP, init_msg_csma);
                    delete msg ;
                    nbDroppedFrames++;
                }
                 if (msg->getKind() == ACK){
                     EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                    EV <<"ACK received";
                    //sendUp(msg);
                     send(msg, upperLayerOut_dual);
                     EV <<"ACK sent";
                  }

                 if (msg->getKind() == ACK_CSMA){
                     EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                      EV <<"ACK CSMA received";
                       //sendUp(msg);
                       send(msg, upperLayerOut_dual);
                       EV <<"ACK_CSMA sent";
                    }
                else if (msg->getArrivalGateId() == lowerLayerInWuR_dual) {
                    delete msg;
                }
                break;



                //******************************************* RX_DATA_STATE************************************
            case RX_DATA_STATE:
                if ((msg->getArrivalGateId() == lowerLayerIn_dual) && (msg->getKind() == DATA_MESSAGE) && isSink==true ){
                    NetwPkt* incoming_data = static_cast<NetwPkt*>(msg );
                    if (incoming_data->getDestAddr() == findHost()->getIndex()) {
                        EV << "FSM_TRANSITION_ANNOUNCEMENT " << msg->getName() << " in " << Radio_STATE << "and arrival Gate is " << msg->getArrivalGate() << "\n";
                        cancelEvent( rx_timeout_msg ) ;
                        recordPacket( PassedMessage :: INCOMING, PassedMessage :: LOWER_DATA , msg);
                        send(msg, upperLayerOut_dual);
                         dual_to_phy->setRadioState(MiximRadio::TX);
                         Radio_STATE = TX_ACK_STATE;
                        scheduleAt(simTime() + TIME_TO_GO_TO_SLEEP, init_msg);
                        //nbRxFrames++;
                    }
                }

                if ((msg->getArrivalGateId() == lowerLayerIn_dual) && (msg->getKind() == SensorAppLayerWuRPCA::DATA_MESSAGE_CSMA) && isSink==true ){
                     NetwPkt* incoming_data = static_cast<NetwPkt*>(msg );
                     if (incoming_data->getDestAddr() == findHost()->getIndex()) {
                         EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                          cancelEvent( rx_timeout_msg ) ;
                          recordPacket( PassedMessage :: INCOMING, PassedMessage :: LOWER_DATA , msg);
                          send(msg, upperLayerOut_dual);
                          dual_to_phy->setRadioState(MiximRadio::TX);
                          Radio_STATE = TX_ACK_STATE;
                          scheduleAt(simTime() + TIME_TO_GO_TO_SLEEP, init_msg_csma);
                          //nbRxFrames++;
                       }
                  }
                else if ((msg->getArrivalGateId () == lowerLayerIn_dual) && (msg->getKind() == WUC_MESSAGE_PCA || msg->getKind() == SensorAppLayerWuRPCA::WUC_MESSAGE_CSMA)) {
                    EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                    delete msg ;
                }
                //  we have a timeout because no received DATA after being activated
                 else if (msg->getKind() == RX_TIMEOUT_MSG_TYPE) {
                     EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                     Radio_STATE = INIT_STATE;
                     scheduleAt(simTime () + TIME_TO_GO_TO_SLEEP, init_msg );
                 }//msg->getKind() == MacToPhyInterface::RADIO_SWITCHING_OVER
                 else if (msg->getKind() == RX_TIMEOUT_MSG_CSMA) {
                     EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                    Radio_STATE = INIT_STATE;
                    scheduleAt(simTime () + TIME_TO_GO_TO_SLEEP, init_msg_csma );
                  }
                 else if  (msg->getKind () == RADIO_SWITCHED) {
                     delete msg;
                 }
                 else if (msg->getArrivalGateId() == lowerLayerInWuR_dual){
                     delete msg;
                 }
                break ;
            case TX_ACK_STATE:
             if (msg->getKind() == ACK_SENT) {
                 EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                 Radio_STATE = INIT_STATE;
                 scheduleAt(simTime () + TIME_TO_GO_TO_SLEEP, init_msg );
             }
             else if (msg->getKind() == ACK_SENT_CSMA) {
                 EV << "Message Name= " << msg->getName() << " in FSM State= " << Radio_STATE << "\n";
                 Radio_STATE = INIT_STATE;
                 scheduleAt(simTime () + TIME_TO_GO_TO_SLEEP, init_msg_csma );
                          }
             break;

                }
            return;


}

void DualRadioPCA::recordPacket(PassedMessage::direction_t dir,
                             PassedMessage::gates_t gate,
                             const cMessage * msg) {
    if (!doStats) return;
    passedMsg->direction = dir;
    passedMsg->gateType = gate;
    passedMsg->kind = msg->getKind();
    passedMsg->name = msg->getName();

}

void DualRadioPCA::handleUpperMsg(cMessage * msg)
    {

    handleSelfMsg(msg);

    }
void DualRadioPCA::handlelowerWuC(cMessage* msg)
    {
    // simply pass the massage as self message, to be processed by the FSM.
        handleSelfMsg(msg);
    }
void DualRadioPCA::handleLowerControl(cMessage* msg)
    {
    switch (msg->getKind())
    {
        case IControl::AWAKE_PCA:
            sendControlUp(msg);
            EV <<"Control message from dual layer sent to Network layer, we are in the dual layer\n";
            break;
        case IControl::SLEEPING_PCA:
            sendControlUp(msg);
            break;
        case IControl::AWAKE_CSMA:
            sendControlUp(msg);
            EV <<"Control message from dual layer sent to Network layer, we are in the dual layer\n";
            break;
        case IControl::SLEEPING_CSMA:
            sendControlUp(msg);
            break;
        default:
            handleSelfMsg(msg);
        }
    }
void DualRadioPCA::handleLowerMsg(cMessage* msg)
    {
    // simply pass the massage as self message, to be processed by the FSM.
        handleSelfMsg(msg);

    }

void DualRadioPCA::handleUpperMsgWuC(cMessage* msg)
    {
    // simply pass the massage as self message, to be processed by the FSM.
        handleSelfMsg(msg);
    }
void DualRadioPCA::handleLowerControlWuC(cMessage* msg)
    {
    // simply pass the massage as self message, to be processed by the FSM.
        handleSelfMsg(msg);
    }


void DualRadioPCA::handleUpperControl(cMessage* msg){
    send(msg, lowerControlOut_dual);
    EV <<"Control message from network layer sent to Nic Contrller, we are in the dual layer\n";
}

void DualRadioPCA::finish() {
    if (stats) {
        recordScalar("nbTxWuC", nbTxWuC);
        recordScalar("nbTxFrames", nbTxFrames);
        recordScalar("nbRxWuC", nbRxWuC);
        recordScalar("nbTxFrames", nbRxFrames);
        recordScalar("nbDroppedFrames", nbDroppedFrames);
        recordScalar("nbDroppedWuC", nbDroppedWuC);

    }
}

DualRadioPCA::~DualRadioPCA()
{
    cancelAndDelete(rx_timeout_msg);
        cancelAndDelete(init_msg);
    //if (doStats) {
        //delete passedMsg;
    //}
}

