package constantin.video.example.decodingperf


data class TestResultDecodingInfo(
        val date: String? = null,
        val time: String? = null,
        val emulator: Boolean ? = null,
        val vs_source: Int? = null,
        val vs_assets_filename: String ?=null,
        //Directly maps to Decoding info
        val currentFPS: Float ?=null,
        val currentKiloBitsPerSecond: Float ?=null,
        val avgParsingTime_ms: Float ?=null,
        val avgWaitForInputBTime_ms: Float ?=null,
        val avgHwDecodingTime_ms: Float ?=null,
        val avgDecodingTime_ms: Float ?=null,
        val numberNalus: Int ?=null,
        val numberNalusFeeded: Int ?=null
)