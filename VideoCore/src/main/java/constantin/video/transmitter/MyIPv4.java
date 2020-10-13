package constantin.video.transmitter;

import java.net.Inet4Address;
import java.util.ArrayList;

// Wrapper around an array of 4 integers range 0...255 forming an IP

public class MyIPv4 {
    public final int[] data=new int[4];

    public MyIPv4(final Inet4Address inet4Address) {
        //data = stringToIp(inet4Address.getHostAddress());
        final byte[] tmp= inet4Address.getAddress();
        for(int i=0;i<4;i++){
            data[i]=(tmp[i] & 0xff);
        }
    }

    // Example: For 192.168.1.1 it returns a list with (192.168.1.0 <-> 192.168.1.255 )
    // But the base address (aka 192.168.1.1 ) is not contained in this list
    //This methods makes some assumptions that might not be true on all devices - but testing is the only
    //way to find out if they work
    public ArrayList<String> getAllSubRangeIpAddressesString(){
        final ArrayList<String> ret=new ArrayList<>();
        for(int i=0;i<256;i++){
            if(i!=data[3]){
                final String s=asString(new int[]{data[0],data[1],data[2],i});
                //System.out.println(s);
                ret.add(s);
            }
        }
        return ret;
    }

    //Returns the 4 digits that make up an ip.
    //Example: 192.168.1.1 -> [192,168,1,1]
    //returns null on failure
    private static int[] stringToIp(final String ip){
        //System.out.println("Ip is "+ip);
        String[] sub = ip.split("\\.");
        //for(int i=0;i<sub.length;i++){
        //    System.out.println("Chunck "+sub[i]);
        //}
        if(sub.length!=4)return null;
        final int[] ret=new int[4];
        for(int i=0;i<4;i++){
            try{
                ret[i]=Integer.parseInt(sub[i]);
            }catch (NumberFormatException e){
                e.printStackTrace();
                return null;
            }
        }
        return ret;
    }

    public static String asString(int[] data) {
        return data[0] + "." + data[1] + "." + data[2] + "." + data[3];
    }

}
