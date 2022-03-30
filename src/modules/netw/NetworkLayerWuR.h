/*
 * NetworkLayerWuR.h
 *
 *  Created on: 26 août 2020
 *      Author: asus
 */

#ifndef NETWORKLAYERWUR_H_
#define NETWORKLAYERWUR_H_

#include "MiXiMDefs.h"
#include "BaseLayer.h"
#include "SimpleAddress.h"
#include "BaseNetwLayer.h"
class ArpInterface;
class NetwPkt;

class MIXIM_API NetworkLayerWuR : public BaseNetwLayer
{

private:
    /** @brief Copy constructor is not allowed.
     */
    NetworkLayerWuR(const NetworkLayerWuR&);
    /** @brief Assignment operator is not allowed.
     */
    NetworkLayerWuR& operator=(const NetworkLayerWuR&);

public:
    /** @brief network packet pointer type. */
    typedef NetwPkt* netwpkt_ptr_t;
    /** @brief Message kinds used by this layer.*/
    enum BaseNetwMessageKinds {
        /** @brief Stores the id on which classes extending BaseNetw should
         * continue their own message kinds.*/
                ACK = 9,
                ACK_CSMA = 17,

        LAST_BASE_NETW_MESSAGE_KIND = 24000,
    };
    /** @brief Control message kinds used by this layer.*/
    enum BaseNetwControlKinds {
        /** @brief Stores the id on which classes extending BaseNetw should
         * continue their own control kinds.*/
        LAST_BASE_NETW_CONTROL_KIND = 24500,

    };

protected:
    /**
     * @brief Length of the NetwPkt header
     * Read from omnetpp.ini
     **/
    int headerLength;

    /** @brief Pointer to the arp module*/
    ArpInterface* arp;

    /** @brief cached variable of my networ address */
    LAddress::L3Type myNetwAddr;

    /** @brief Enables debugging of this module.*/
    bool coreDebug;

public:
    //Module_Class_Members(BaseNetwLayer,BaseLayer,0);
    NetworkLayerWuR()
      : BaseNetwLayer()
      , headerLength(0)
      , arp(NULL)
      , myNetwAddr()
      , coreDebug(false)
    {}

    NetworkLayerWuR(unsigned stacksize)
      : BaseNetwLayer(stacksize)
      , headerLength(0)
      , arp(NULL)
      , myNetwAddr()
      , coreDebug(false)
    {}

    /** @brief Initialization of the module and some variables*/
    virtual void initialize(int);

  protected:
    /**
     * @name Handle Messages
     * @brief Functions to redefine by the programmer
     *
     * These are the functions provided to add own functionality to your
     * modules. These functions are called whenever a self message or a
     * data message from the upper or lower layer arrives respectively.
     *
     **/
    /*@{*/

    /** @brief Handle messages from upper layer */
    virtual void handleUpperMsg(cMessage* msg);

    /** @brief Handle messages from lower layer */
    virtual void handleLowerMsg(cMessage* msg);

    /** @brief Handle self messages */
    virtual void handleSelfMsg(cMessage* /*msg*/){
    error("BaseNetwLayer does not handle self messages");
    };

    /** @brief Handle control messages from lower layer */
    virtual void handleLowerControl(cMessage* msg);

    /** @brief Handle control messages from lower layer */
    virtual void handleUpperControl(cMessage* msg);

    /*@}*/

    /** @brief decapsulate higher layer message from NetwPkt */
    virtual cMessage*     decapsMsg(netwpkt_ptr_t);

    /** @brief Encapsulate higher layer packet into an NetwPkt*/
    virtual netwpkt_ptr_t encapsMsg(cPacket*);

    /**
     * @brief Attaches a "control info" (NetwToMac) structure (object) to the message pMsg.
     *
     * This is most useful when passing packets between protocol layers
     * of a protocol stack, the control info will contain the destination MAC address.
     *
     * The "control info" object will be deleted when the message is deleted.
     * Only one "control info" structure can be attached (the second
     * setL3ToL2ControlInfo() call throws an error).
     *
     * @param pMsg      The message where the "control info" shall be attached.
     * @param pDestAddr The MAC address of the message receiver.
     */
    virtual cObject* setDownControlInfo(cMessage *const pMsg, const LAddress::L2Type& pDestAddr);
    /**
     * @brief Attaches a "control info" (NetwToUpper) structure (object) to the message pMsg.
     *
     * This is most useful when passing packets between protocol layers
     * of a protocol stack, the control info will contain the destination MAC address.
     *
     * The "control info" object will be deleted when the message is deleted.
     * Only one "control info" structure can be attached (the second
     * setL3ToL2ControlInfo() call throws an error).
     *
     * @param pMsg      The message where the "control info" shall be attached.
     * @param pSrcAddr  The MAC address of the message receiver.
     */
    virtual cObject* setUpControlInfo(cMessage *const pMsg, const LAddress::L3Type& pSrcAddr);

};



#endif /* NETWORKLAYERWUR_H_ */
