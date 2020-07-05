package constantin.video.core;

import android.util.ArrayMap;

import java.util.Map;

@SuppressWarnings("WeakerAccess")
public class DecodingInfo {
    public final float currentFPS;
    public final float currentKiloBitsPerSecond;
    public final float avgParsingTime_ms;
    public final float avgWaitForInputBTime_ms;
    public final float avgHWDecodingTime_ms; //time the hw decoder was holding on to frames. Not the full decoding time !
    public final float avgTotalDecodingTime_ms;
    public final int nNALU;
    public final int nNALUSFeeded;

    public DecodingInfo(){
        currentFPS=0;
        currentKiloBitsPerSecond=0;
        avgParsingTime_ms=0;
        avgWaitForInputBTime_ms=0;
        avgHWDecodingTime_ms=0;
        nNALU=0;
        nNALUSFeeded=0;
        avgTotalDecodingTime_ms =0;
    }

    public DecodingInfo(float currentFPS, float currentKiloBitsPerSecond,float avgParsingTime_ms,float avgWaitForInputBTime_ms,float avgHWDecodingTime_ms,
                        int nNALU,int nNALUSFeeded){
        this.currentFPS=currentFPS;
        this.currentKiloBitsPerSecond=currentKiloBitsPerSecond;
        this.avgParsingTime_ms=avgParsingTime_ms;
        this.avgWaitForInputBTime_ms=avgWaitForInputBTime_ms;
        this.avgHWDecodingTime_ms =avgHWDecodingTime_ms;
        this.avgTotalDecodingTime_ms =avgParsingTime_ms+avgWaitForInputBTime_ms+avgHWDecodingTime_ms;
        this.nNALU=nNALU;
        this.nNALUSFeeded=nNALUSFeeded;
    }

    public Map<String,Object> toMap(){
        Map<String, Object> decodingInfo = new ArrayMap<>();
        decodingInfo.put("currentFPS",currentFPS);
        decodingInfo.put("currentKiloBitsPerSecond",currentKiloBitsPerSecond);
        decodingInfo.put("avgParsingTime_ms",avgParsingTime_ms);
        decodingInfo.put("avgWaitForInputBTime_ms",avgWaitForInputBTime_ms);
        decodingInfo.put("avgHWDecodingTime_ms", avgHWDecodingTime_ms);
        decodingInfo.put("avgTotalDecodingTime_ms", avgTotalDecodingTime_ms);
        decodingInfo.put("nNALU",nNALU);
        decodingInfo.put("nNALUSFeeded",nNALUSFeeded);
        return decodingInfo;
    }

    public String toString(final boolean newline){
        final StringBuilder builder=new StringBuilder();
        builder.append( "Decoding info:\n");
        final Map<String,Object> map=toMap();
        for(final String key:map.keySet()){
            //Either float or int, toString available
            final Object value=map.get(key);
            builder.append(key).append(":").append(value);
            if(newline)builder.append("\n");
        }
        return builder.toString();
    }

    @Override
    public String toString() {
        return toString(false);
    }
}
