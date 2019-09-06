package constantin.video.core;

@SuppressWarnings("WeakerAccess")
public class DecodingInfo {
    public final float currentFPS;
    public final float currentKiloBitsPerSecond;
    public final float avgParsingTime_ms;
    public final float avgWaitForInputBTime_ms;
    public final float avgDecodingTime_ms;
    public final int nNALU;
    public final int nNALUSFeeded;


    public DecodingInfo(float currentFPS, float currentKiloBitsPerSecond,float avgParsingTime_ms,float avgWaitForInputBTime_ms,float avgDecodingTime_ms,
                        int nNALU,int nNALUSFeeded){
        this.currentFPS=currentFPS;
        this.currentKiloBitsPerSecond=currentKiloBitsPerSecond;
        this.avgParsingTime_ms=avgParsingTime_ms;
        this.avgWaitForInputBTime_ms=avgWaitForInputBTime_ms;
        this.avgDecodingTime_ms=avgDecodingTime_ms;
        this.nNALU=nNALU;
        this.nNALUSFeeded=nNALUSFeeded;
    }


    @Override
    public String toString() {
        return "Decoding info.>"+
                " currentFPS:"+currentFPS+
                " currentKiloBitsPerSecond:"+currentKiloBitsPerSecond+
                " avgParsingTime_ms:"+avgParsingTime_ms+
                " avgWaitForInputBTime_ms:"+avgWaitForInputBTime_ms+
                " avgDecodingTime_ms:"+avgDecodingTime_ms+
                " totalDecodingTime_ms"+(avgParsingTime_ms+avgWaitForInputBTime_ms+avgDecodingTime_ms)+
                " nNALU:"+nNALU+
                " nNALUSFeeded:"+nNALUSFeeded;
    }
}
