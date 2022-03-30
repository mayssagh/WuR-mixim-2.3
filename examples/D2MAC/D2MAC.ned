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

import org.mixim.base.connectionManager.ConnectionManager;
import org.mixim.modules.connectionManager.UnitDisk;
import org.mixim.base.modules.BaseWorldUtility;

network D2MAC {
    
    parameters:
        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)
        double numHosts; // total number of hosts in the network

        @display("bgb=$playgroundSizeX,$playgroundSizeY,white,,;bgp=10,50");
        
        submodules:
        world: BaseWorldUtility {
            parameters:
                playgroundSizeX = playgroundSizeX;
                playgroundSizeY = playgroundSizeY;
                playgroundSizeZ = playgroundSizeZ;
                @display("p=100,30;i=misc/globe");

        }
        connectionManager: UnitDisk {
         parameters:
               @display("p=200,30;b=42,42,rect,yellow,,;i=abstract/multicast");
              //@display("p=200,50;b=42,42,rect,yellow,,;i=abstract/multicast");

       }
       
       
        node[numHosts]: Host{
            parameters:
                numHosts = numHosts;
                //@display("p=50,80;b=42,42,rect,white,,;i=device/wifilaptop_vs");
                @display("p=50,80;i=device/wifilaptop_vs");

        }
        
       // sink: Sink {
          //  parameters:
              // @display("p=150,160;i=device/wifilaptop");
            //gates:
              //  g[numHosts];
       // }
    connections allowunconnected:
       
       for i = 0..numHosts-2 {
node[i].gate++ <--> node[i+1].gate++;
       }
      
        
}
    