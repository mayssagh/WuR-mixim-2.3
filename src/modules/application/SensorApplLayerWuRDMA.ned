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

package org.mixim.modules.application;
import org.mixim.base.modules.IBaseApplLayer;
//
// TODO auto-generated module
//
simple SensorApplLayerWuRDMA like IBaseApplLayer
{
     parameters:
        bool debug = default(false); // debug switch
        bool stats = default(true);   // stats generation switch
        bool trace = default(true);  // activates detailed logging (per source latencies and vector logging). stats must be true.
        bool broadcastPackets = default(false); // send packets in broadcast mode
        double nbPacketsCSMA = default(0);
        double nbPacketsPCA = default(0);
        int destAddr = default(0);
        string trafficType = default("periodic");  	// Can be one of: periodic, uniform or exponential
        //the given mean (that is, with parameter lambda=1/mean).
        double trafficParam @unit(s) = default(0.5 s);			// the mean time between two packets
        double initializationTime @unit(s) = default( 0 s); // minimum time before generation of the first packet
		int headerLength @unit(byte) = default(0 byte);
        bool notAffectedByHostState = default(true);
        int packetLength @unit(byte) = default(35 byte);//35
        int WuCLength @unit(byte) = default(4 byte); //4 byte
        //int packetLength @unit("bit")=280 bit; // length of the generated packet (in bits)
        //int WuCLength @unit("bit")= 32 bit;
        double WuCTime @unit(s)= 0.006s;//0.00586
        double TimeON @unit(s)=0.00179s;
    gates:
        input lowerLayerIn; // from network layer
        input lowerControlIn;
        output lowerLayerOut; // to network layer
        output lowerControlOut;
}
