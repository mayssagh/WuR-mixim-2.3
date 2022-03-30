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

package org.mixim.examples.D2MAC;

import org.mixim.base.modules.BaseWorldUtility;
import org.mixim.modules.application.BurstApplLayer;
import org.mixim.modules.application.SensorAppLayerWuRPCA;
import org.mixim.modules.application.SensorApplLayerWuRDMA;
import org.mixim.base.modules.BaseNetwLayer;
import org.mixim.modules.netw.NetworkLayerWuR;
import org.mixim.modules.dual.DualRadioPCA;
import org.mixim.modules.netw.ArpHost;
import org.mixim.modules.power.battery.SimpleBattery;
import org.mixim.modules.power.battery.BatteryStats;
import org.mixim.modules.nic.MainNic2;
import org.mixim.modules.nic.NicWakeUpRadioPCA;
import org.mixim.modules.nic.WuRNic;

module Host
{
    parameters:
        @display("bgb=367,300,white");
        double numHosts; // total number of hosts in the network
        string mobilityType = default("StationaryMobility"); //type of the mobility module
        @node();
    gates:
        inout g;
        inout gate[];
        input upperLayerIn_dual; //1
        output upperLayerOut_dual; //2
        input lowerLayerIn_dual;//3
        output lowerLayerOut_dual;//4
        input upperControlIn_dual;//5
        output upperControlOut_dual;//6
        input lowerControlIn_dual;//7
        output lowerControlOut_dual;//8
        input lowerLayerInWuR_dual; // from upper layer//9
        output upperControlOutWuR_dual; // to uppe layer//10
        input lowerControlInWuR_dual;//11
        output lowerControlOutWuR_dual; // control to upper layer//12
        input upperLayerInWuR_dual;//13
    output upperLayerOutWuR_dual ;//14
    output lowerLayerOutWuR_dual;//15
    input upperControlInWuR_dual; //16
        output upperLayerOut;
        input upperLayerIn;
        input MainRadioIn;
        output MainRadioOut;
        input WuRRadioIn;
        output WuRRadioOut;
    submodules:
       
        mobility: <mobilityType> like inet.mobility.IMobility {
            parameters:
                @display("p=238,40;b=24,24,rect,blue;i=block/cogwheel");
        }
        arp: ArpHost {
            parameters:
                @display("p=238,112;b=24,24,rect,blue;i=block/network2");
        }
        battery: SimpleBattery {
            parameters:
                @display("p=238,184;b=24,24,rect,blue;i=block/plug, #FF8C00");
        }
        batteryStats: BatteryStats {
            parameters:
                @display("p=238,254;b=24,24,rect,blue;i=block/table,#ffaa61");
        }
        MainNic: MainNic2 {
            parameters:
                @display("p=60,240;i=block/wrxtx,#008000");
        }
        WuRNic: WuRNic {
            parameters:
                @display("p=148,240;i=block/wrxtx,#008000");
        }
        control: DualRadioPCA {
            parameters:
                //@display("p=104,170;i=block/control2,#4f4f4f");
                @display("p=104,170");
        }
        netwl: NetworkLayerWuR {
            parameters:
                @display("p=104,100;i=block/layer");
        }
        appl: SensorApplLayerWuRDMA {
            parameters:
                @display("p=104,30;i=block/app");
        }
    connections allowunconnected:
        netwl.upperLayerOut --> appl.lowerLayerIn;
        netwl.upperLayerIn <-- appl.lowerLayerOut;
        netwl.upperControlOut --> appl.lowerControlIn;
        netwl.upperControlIn <-- appl.lowerControlOut;

        control.upperLayerOut_dual --> netwl.lowerLayerIn;
        control.upperLayerIn_dual <-- netwl.lowerLayerOut;
        control.upperControlOut_dual --> netwl.lowerControlIn;
        control.upperControlIn_dual <-- netwl.lowerControlOut;


         // Main Radio
        MainNic.upperLayerOut --> control.lowerLayerIn_dual;
        MainNic.upperLayerIn <-- control.lowerLayerOut_dual;
        MainNic.upperControlOut --> control.lowerControlIn_dual;
        MainNic.upperControlIn <-- control.lowerControlOut_dual;

        MainRadioIn --> { @display("m=s"); } --> MainNic.radioIn;

         // WuR Radio
        WuRNic.upperLayerOut --> control.lowerLayerInWuR_dual;
        WuRNic.upperLayerIn <-- control.lowerLayerOutWuR_dual;
        WuRNic.upperControlOut --> control.lowerControlInWuR_dual;
        WuRNic.upperControlIn <-- control.lowerControlOutWuR_dual;

        WuRRadioIn --> WuRNic.radioIn;






}
