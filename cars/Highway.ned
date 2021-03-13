// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
//
package bigdou.cars;

import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.inet.AdhocHost;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import inet.node.ethernet.Eth10G;

import lte.world.radio.LteChannelControl;
import lte.epc.PgwStandardSimplified;
import lte.corenetwork.binder.LteBinder;
import lte.corenetwork.nodes.eNodeB;
import lte.corenetwork.nodes.Ue;
import lte.corenetwork.nodes.cars.Car;

import org.car2x.veins.subprojects.veins_inet.VeinsInetManager;


network Highway
{
    parameters:
        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)
        @display("bgb=2100,1100");

    submodules:

        routingRecorder: RoutingTableRecorder {
            @display("p=151.37329,711.1689;is=s");
        }
        configurator: IPv4NetworkConfigurator {
            @display("p=151.37329,225.63188");
            config = xmldoc("demo.xml");
        }

        //# Veins manager module
        veinsManager: VeinsInetManager {
            @display("p=154.22939,391.28568;is=s");
        }

        //# LTE modules
        channelControl: LteChannelControl {
            @display("p=154.22939,74.2586;is=s");
        }
        binder: LteBinder {
            @display("p=151.37329,548.37115;is=s");
        }
        server: StandardHost {
            @display("p=999.63495,74.2586;is=n;i=device/server");
        }
        router: Router {
            @display("p=791.13965,71.4025;i=device/smallrouter");
        }
        pgw: PgwStandardSimplified {
            nodeType = "PGW";
            @display("p=591.21265,74.2586;is=l");
        }
        eNB: eNodeB {
            @display("p=791.13965,334.1637;is=vl");
        }
        hNB: Car {
            @display("p=789,389.24;i=device/pocketpc");
        }

    connections allowunconnected:
        server.pppg++ <--> Eth10G <--> router.pppg++;
        router.pppg++ <--> Eth10G <--> pgw.filterGate;
        pgw.pppg++ <--> Eth10G <--> eNB.ppp;
//        pgw.pppg++ <--> Eth10G <--> eNodeB2.ppp;

        //# X2 connections
}
