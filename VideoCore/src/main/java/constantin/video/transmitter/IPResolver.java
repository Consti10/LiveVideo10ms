package constantin.video.transmitter;

import android.content.Context;
import android.util.Log;

import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
import java.util.List;

@SuppressWarnings("WeakerAccess")
public class IPResolver {
    private static final String TAG="IPResolver";

    // Find all possible client IP addresses
    public static List<String> findAllPossibleClientIps(final Context c){
        final ArrayList<String> ipsToTest=getIpsToPing();
       final List<String> allReachableAddresses= HelperPingIP.pingAllIPsMultiThreaded(ipsToTest);
        for(final String ip:allReachableAddresses){
            System.out.println("Found ip:"+ip);
        }
        return allReachableAddresses;
    }

    private static ArrayList<String> getIpsToPing(){
        final ArrayList<Inet4Address> hotspotIp=getWifiOrTetheringIpAddresses();
        final ArrayList<String> ret=new ArrayList<>();
        for(final Inet4Address address:hotspotIp){
            System.out.println("Possible range(s) are "+address);
            ret.addAll(new MyIPv4(address).getAllSubRangeIpAddressesString());
        }
       return ret;
    }

    // Returns a list of all IP addresses that are either WIFI or WIFI hotspot or TETHERING
    private static ArrayList<Inet4Address> getWifiOrTetheringIpAddresses(){
        final ArrayList<Inet4Address> addresses=new ArrayList<>();
        try{
            final Enumeration<NetworkInterface> networkInterfacesEnumeration=NetworkInterface.getNetworkInterfaces();
            while (networkInterfacesEnumeration.hasMoreElements()){
                final NetworkInterface networkInterface=networkInterfacesEnumeration.nextElement();
                if(!networkInterface.isUp() || networkInterface.getName().contains("dummy0") || networkInterface.isLoopback()){
                    continue;
                }
                //printNetworkInterface(networkInterface);
                if(networkInterface.getName().contains("wlan") || networkInterface.getName().contains("rndis")){
                    addresses.add(getInet4AddressOfNetworkInterface(networkInterface));
                }
            }
            return addresses;
        }catch(java.net.SocketException e){
            e.printStackTrace();
        }
        throw new RuntimeException("getWifiOrTetheringIpAddresses - should never happen");
    }

    // Helper that prints a network interface and its sub inet addresses
    private static void printNetworkInterface(final NetworkInterface networkInterface){
        StringBuilder b=new StringBuilder();
        b.append("DisplayName ").append(networkInterface.getDisplayName()).append("\n");
        //b.append("Name ").append(networkInterface.getName()).append("\n");
        Enumeration<InetAddress> inetAddresses = networkInterface.getInetAddresses();
        for (InetAddress inetAddress : Collections.list(inetAddresses)) {
            b.append("InetAddress: ").append(inetAddress).append("\n");
        }
        System.out.println("NI "+b.toString());
    }

    // Only works if the Network interface has exactly one IPv4 address attached
    // This seems to be the case for all NetworkInterfaces i need to take into consideration
    // log warning otherwise
    private static Inet4Address getInet4AddressOfNetworkInterface(final NetworkInterface networkInterface){
        final ArrayList<Inet4Address> addresses=new ArrayList<>();
        final Enumeration<InetAddress> inetAddressesEnumeration=networkInterface.getInetAddresses();
        while (inetAddressesEnumeration.hasMoreElements()){
            InetAddress inetAddress=inetAddressesEnumeration.nextElement();
            if(inetAddress instanceof Inet4Address){
                addresses.add((Inet4Address)inetAddress);
            }
        }
        if(addresses.size()!=1){
            printNetworkInterface(networkInterface);
            throw new RuntimeException("addresses.size()!=1 ");
        }
        Log.d(TAG,"Found proper IP for Network interface "+networkInterface.getDisplayName());
        return addresses.get(0);
    }
}
