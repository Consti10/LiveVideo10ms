package constantin.video.core;

public class ThreadPriorityTester {

    // Create 2 Threads. Both set their own priority, but to a different value.
    public static void test(){
        final Runnable runnable1=new Runnable() {
            @Override
            public void run() {
                while (true){
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    System.out.println("1Thread prio is"+android.os.Process.getThreadPriority(android.os.Process.myTid()));
                    android.os.Process.setThreadPriority(-19);
                    System.out.println("1Thread prio is"+android.os.Process.getThreadPriority(android.os.Process.myTid()));
                }
            }
        };
        final Runnable runnable2=new Runnable() {
            @Override
            public void run() {
                while (true){
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    System.out.println("2Thread prio is"+android.os.Process.getThreadPriority(android.os.Process.myTid()));
                    android.os.Process.setThreadPriority(-10);
                    System.out.println("2Thread prio is"+android.os.Process.getThreadPriority(android.os.Process.myTid()));
                }
            }
        };
        Thread thread1=new Thread(runnable1);
        thread1.setName("Test Prio 1");
        thread1.start();
        Thread thread2=new Thread(runnable2);
        thread2.setName("Test Prio 2");
        thread2.start();
    }

    public static void test2(){
        final Runnable runnable=new Runnable() {
            @Override
            public void run() {
                while (true){
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    System.out.println("Process Thread prio is"+android.os.Process.getThreadPriority(android.os.Process.myTid()));
                    System.out.println("Thread prio is"+Thread.currentThread().getPriority());
                    android.os.Process.setThreadPriority(-19);
                    System.out.println("Process Thread prio is"+android.os.Process.getThreadPriority(android.os.Process.myTid()));
                    System.out.println("Thread prio is"+Thread.currentThread().getPriority());
                }
            }
        };
        Thread thread1=new Thread(runnable);
        thread1.setName("Test Prio 1");
        thread1.start();
    }
}
