# Telemetry

## TelemetryCore 
contains the core library, written in cpp/c. TelemetryReceiver.cpp is the main object.
It parses telemetry coming from different uav platforms ( MAVLINK,LTM,FRSKY ) into one uniform data model and provides an interface
for optaining Telemetry values as human-readable strings (e.g. for display on an On-Screen-Display) without being platform-dependent.
Its functions are also exposed to JAVA code using the ndk. 


### TelemetryExample (merged into VideoExample)
contains just a MainActivity.java class that allows modification of settings and lists debugging values on TextViews.
These debugging values also include all received data as prefix - value - metric triples. Note that some prefixes have to be interpreted
as Icons instead of utf-8 strings, and therefore are not displayed properly in the example.
