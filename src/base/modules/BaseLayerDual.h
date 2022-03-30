/*
 * BaseLayerDual.h
 *
 *  Created on: 19 août 2020
 *      Author: asus
 */

#ifndef BASELAYERDUAL_H_
#define BASELAYERDUAL_H_

#include <BaseModule.h>
#include "IControl.h"
#include "Controller.h"


class MIXIM_API BaseLayerDual : public BaseModule {
    public:
    /** @brief SignalID for packets. */
    const static simsignalwrap_t catPacketSignal;
    /** @brief Signal for passed messages.*/
    const static simsignalwrap_t catPassedMsgSignal;
    /** @brief Signal for dropped packets.*/
    const static simsignalwrap_t catDroppedPacketSignal;
    private:
    /** @brief Copy constructor is not allowed.
     */
    BaseLayerDual(const BaseLayerDual&);
    /** @brief Assignment operator is not allowed.
     */
    BaseLayerDual& operator=(const BaseLayerDual&);
public:
    BaseLayerDual()
            : BaseModule()
            , upperLayerIn_dual(-1)
            , upperLayerOut_dual(-1)
            , lowerLayerIn_dual(-1)
            , lowerLayerOut_dual(-1)
            , upperControlIn_dual(-1)
            , upperControlOut_dual(-1)
            , lowerControlIn_dual(-1)
            , lowerControlOut_dual(-1)
            , lowerLayerInWuR_dual(-1)
            , upperControlOutWuR_dual(-1)
            , lowerControlInWuR_dual(-1)
            , lowerControlOutWuR_dual(-1)
            , upperLayerInWuR_dual(-1)
            , upperLayerOutWuR_dual(-1)
            , lowerLayerOutWuR_dual(-1)
            , upperControlInWuR_dual(-1)
        {}
        BaseLayerDual(unsigned stacksize)
            : BaseModule(stacksize)
        , upperLayerIn_dual(-1)
                    , upperLayerOut_dual(-1)
                    , lowerLayerIn_dual(-1)
                    , lowerLayerOut_dual(-1)
                    , upperControlIn_dual(-1)
                    , upperControlOut_dual(-1)
                    , lowerControlIn_dual(-1)
                    , lowerControlOut_dual(-1)
                    , lowerLayerInWuR_dual(-1)
                    , upperControlOutWuR_dual(-1)
                    , lowerControlInWuR_dual(-1)
                    , lowerControlOutWuR_dual(-1)
                    , upperLayerInWuR_dual(-1)
                    , upperLayerOutWuR_dual(-1)
                    , lowerLayerOutWuR_dual(-1)
                    , upperControlInWuR_dual(-1)
        {}
    virtual ~BaseLayerDual() {};
    virtual void initialize(int);


    /**
         * @brief Return a pointer to the primary interface.  Returns 0 if the
         * interface is not controllable.
         */
   // virtual IControllable* getPrimaryIface() const {return primaryController;};
    /**
         * The basic handle message function.
         *
         * Depending on the gate a message arrives handleMessage just calls
         * different handle*Msg functions to further process the message.
         *
         * You should not make any changes in this function but implement all
         * your functionality into the handle*Msg functions called from here.
         *
         * @sa handleUpperMsg, handleLowerMsg, handleSelfMsg
         **/
    virtual void handleMessage(cMessage* msg);

    /**
         * Convenience functions for sending to out-gates.
         */
    void sendDown(cMessage *msg);
    void sendUp(cMessage *msg);
    void sendControlUp(cMessage *msg);
    void sendControlDown(cMessage *msg);
    void sendDownWuC(cMessage *msg);
    void sendUpWuC(cMessage *msg);
    void sendControlUpWuC(cMessage *msg);
    void sendControlDownWuC(cMessage *msg);




    virtual const int mydualAddr() {
        cModule* parent = getParentModule();
        return parent->getIndex();
    };

protected:

    /** @name gate ids*/
    /*@{*/
    int upperLayerIn_dual;
    int upperLayerOut_dual;
    int lowerLayerIn_dual;
    int lowerLayerOut_dual;
    int upperControlIn_dual;
    int upperControlOut_dual;
    int lowerControlIn_dual;
    int lowerControlOut_dual;
    int lowerLayerInWuR_dual;
    int upperControlOutWuR_dual;
    int lowerControlInWuR_dual;
    int lowerControlOutWuR_dual;
    int upperLayerInWuR_dual;
    int upperLayerOutWuR_dual;
    int lowerLayerOutWuR_dual;
    int upperControlInWuR_dual;

    int headerLength;
    int primaryNicId;

    Controller* primaryController;



    /** @brief Handle self messages such as timer... */
        virtual void handleSelfMsg(cMessage* msg) = 0;

        /**
         * Handler routines for incoming messages
         */
        virtual void handleUpperMsg(cMessage* msg) = 0;
        virtual void handlelowerWuC(cMessage* msg) = 0;
        virtual void handleLowerControl(cMessage* msg) = 0;
        virtual void handleLowerMsg(cMessage* msg) = 0;
        virtual void handleUpperControl(cMessage* msg) = 0;

        virtual void handleUpperMsgWuC(cMessage* msg) = 0;
        //virtual void handleLowerMsgWuR(cMessage* msg) = 0;
        virtual void handleLowerControlWuC(cMessage* msg) = 0;

    /** @brief Called when the state of the primary NIC changes */
   // virtual void primaryNicStateUpdate(IControllable::Status newState) = 0;

        /** @brief Called when the state of the secondary NIC changes */
        //virtual void secondaryNicStateUpdate(IControllable::Status newState) = 0;


};




#endif /* BASELAYERDUAL_H_ */
