package constantin.video.core;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.widget.Toast;

import java.net.InetAddress;
import java.net.NetworkInterface;
import java.util.Enumeration;

//Created for FPV-VR by Constantin
//Helps with USB status / start USB tethering

public final class IsConnected {

    public enum USB_CONNECTION{
        NOTHING,DATA,TETHERING
    }

    private static boolean checkWifiConnectedToNetwork(final Context context,final String networkName){
        WifiManager wifiManager = (WifiManager) context.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        WifiInfo wifiInfo;
        if(wifiManager==null){
            return false;
        }
        wifiInfo = wifiManager.getConnectionInfo();
        if (wifiInfo.getSupplicantState() == SupplicantState.COMPLETED ) {
            String ssid = wifiInfo.getSSID();
            return ssid.equals(networkName);
        }
        return false;
        //System.out.println(wifiInfo.getSupplicantState().toString());
    }

    public static boolean checkWifiConnectedOpenHD(final Context context){
        return checkWifiConnectedToNetwork(context,"\"Open.HD\"");
    }

    public static boolean checkWifiConnectedTest(final Context context){
        return checkWifiConnectedToNetwork(context,"\"TestAero\"");
    }

    public static boolean checkWifiConnectedEZWB(final Context context){
        return checkWifiConnectedToNetwork(context,"\"EZ-WifiBroadcast\"");
    }


    public static USB_CONNECTION getUSBStatus(Context context){
        final Intent intent = context.registerReceiver(null, new IntentFilter("android.hardware.usb.action.USB_STATE"));
        if(intent!=null){
            final Bundle extras=intent.getExtras();
            if(extras!=null){
                final boolean connected=extras.getBoolean("connected",false);
                final boolean tetheringActive=extras.getBoolean("rndis",false);
                if(tetheringActive){
                    return USB_CONNECTION.TETHERING;
                }
                if(connected){
                    return USB_CONNECTION.DATA;
                }
            }
        }
        //UsbManager manager=(UsbManager)context.getSystemService(Context.USB_SERVICE);
        //manager.getClass().getDeclaredMethods().
        return USB_CONNECTION.NOTHING;
    }


    public static String getUSBTetheringLoopbackAddress(){
        try{
            for(Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements();){
                final NetworkInterface intf=en.nextElement();
                //System.out.println("Intf:"+intf.getName());
                if(intf.getName().contains("rndis")){
                    final Enumeration<InetAddress> inetAdresses=intf.getInetAddresses();
                    while (inetAdresses.hasMoreElements()){
                        final InetAddress inetAddress=inetAdresses.nextElement();
                        System.out.println(inetAddress.toString());
                        //if(inetAddress.isLoopbackAddress()){
                            return inetAddress.toString().replace("/","");
                        //}
                    }
                }
            }
        }catch(Exception e){e.printStackTrace();}
        return null;
    }

    public static int getLastNumberOfIP(final String ip){
        String[] parts = ip.split(".");
        assert (parts.length==4);
        return Integer.parseInt(parts[3]);
    }

    public static void openUSBTetherSettings(final Context c){
        final Intent intent = new Intent(Intent.ACTION_MAIN, null);
        intent.addCategory(Intent.CATEGORY_LAUNCHER);
        final ComponentName cn = new ComponentName("com.android.settings", "com.android.settings.TetherSettings");
        intent.setComponent(cn);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        Toast.makeText(c, "enable 'USB tethering' (not wifi,but usb hotspot)", Toast.LENGTH_LONG).show();
        c.startActivity(intent);
    }

}
