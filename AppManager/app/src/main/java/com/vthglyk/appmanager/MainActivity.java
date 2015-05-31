package com.vthglyk.appmanager;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.ListActivity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.AsyncTask;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Toast;

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.ListIterator;
import java.util.Timer;
import java.util.TimerTask;


public class MainActivity extends ListActivity {

    public final static String EXTRA_MESSAGE = "com.vthglyk.appmanager.MESSAGE";
    private ArrayList<App> apps;
    private AppManagerAdapter adapter;
    private PackageManager pm;
    private ActivityManager activityManager;

    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        displayRunningApps();

        new Thread(new Runnable() {
            public void run() {

                while(true) {
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    refreshList();

                    runOnUiThread(new Runnable() {
                        public void run() {
                            adapter.notifyDataSetChanged();
                        }

                    });
                }
            }
        }).start();
        /*Timer timer = new Timer();
        timer.scheduleAtFixedRate( new TimerTask() {
            public void run() {

                try{

                    new refreshTask().execute();

                }
                catch (Exception e) {
                    // TODO: handle exception
                }

            }
        }, 500, 500);*/


        // I used that to check if new threads are recognised as new apps
        //The answer is NO

        /*Thread myThread = new Thread(new Runnable() {
            @Override
            public void run() {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(getApplicationContext(), "Started", Toast.LENGTH_SHORT).show();
                    }
                });

                try {
                    Thread.sleep(15000);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(getApplicationContext(), "Finished", Toast.LENGTH_SHORT).show();
                    }
                });
            }
        });
        myThread.setDaemon(true);
        myThread.start();*/
    }

    private void displayRunningApps() {

        apps = new ArrayList<>();
        activityManager = (ActivityManager) this.getSystemService( ACTIVITY_SERVICE );
        pm = this.getPackageManager();
        List<ActivityManager.RunningAppProcessInfo> runningApps = activityManager.getRunningAppProcesses();

        for(ActivityManager.RunningAppProcessInfo running_process : runningApps) {
            try{
                CharSequence c = pm.getApplicationLabel(
                        pm.getApplicationInfo(running_process.processName, PackageManager.GET_META_DATA));
                apps.add(getApp(c.toString(), running_process.pid, running_process.uid));

            }catch(Exception e) {
                //Name Not Found Exception
            }

        }

        adapter = new AppManagerAdapter(this,
                R.layout.row_layout, apps);
        setListAdapter(adapter);
    }

    private App getApp(String s, int p, int u) {
        return new App(s, p, u);
    }

    /** Called when the user clicks the Confirm button */
    public void sendMessage(View view) {
        /*Intent intent = new Intent(this, SendPids.class);
        StringBuilder message = new StringBuilder("The following are high priority applications:\n\n");

        for(App app_help : apps){
            if(app_help.getChecked())
                message.append(app_help.getName()).append("(").append(String.valueOf(app_help.getPid())).append(")\n");
        }
        intent.putExtra(EXTRA_MESSAGE, message.toString());
        startActivity(intent);*/

        Intent intent = new Intent(this, SendPids.class);
        ArrayList<Integer> pidsToSend = new ArrayList<>();
        int H_table_dim = 0;

        for(App app_help : apps){
            if(app_help.getChecked()) {
                pidsToSend.add(app_help.getPid());
                H_table_dim++;
            }
        }

        int[] array = new int[pidsToSend.size()];
        for (int i=0; i < array.length; i++)
        {
            array[i] = pidsToSend.get(i).intValue();
        }

        intent.putExtra("pids", array);
        intent.putExtra("H_table_dim", H_table_dim);
        startActivity(intent);
    }

    /** Called when the user clicks the Refresh button */
    public void refreshList() {
        ArrayList<App> apps_update = new ArrayList<>();
        List<ActivityManager.RunningAppProcessInfo> runningApps = activityManager.getRunningAppProcesses();

        for(ActivityManager.RunningAppProcessInfo running_process : runningApps) {

            try{
                CharSequence c = pm.getApplicationLabel(
                        pm.getApplicationInfo(running_process.processName, PackageManager.GET_META_DATA));
                apps_update.add(getApp(c.toString(), running_process.pid, running_process.uid));

            }catch(Exception e) {
                //Name Not Found Exception
            }

        }

        for(App currentApp : apps_update){
            int index = apps.indexOf(currentApp);

            if(index >= 0 && apps.get(index).getChecked()) {
                currentApp.setChecked(true);
            }
        }
        apps.clear();
        apps.addAll(apps_update);
        //adapter.notifyDataSetChanged();

    }

    /*private class refreshTask extends AsyncTask<Void, Void, Integer> {

        protected Integer doInBackground(Void... nothing) {
            try {
                Thread.sleep(20000);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            ArrayList<App> apps_update = new ArrayList<>(apps);
            apps.clear();
            List<ActivityManager.RunningAppProcessInfo> runningApps = activityManager.getRunningAppProcesses();

            for(ActivityManager.RunningAppProcessInfo running_process : runningApps) {
                try{
                    CharSequence c = pm.getApplicationLabel(
                            pm.getApplicationInfo(running_process.processName, PackageManager.GET_META_DATA));
                    apps.add(getApp(c.toString(), running_process.pid, running_process.uid));

                }catch(Exception e) {
                    //Name Not Found Exception
                }

            }

            for(App currentApp : apps_update) {
                if(currentApp.getChecked()) {
                    int index = apps.indexOf(currentApp);

                    if(index >= 0) {
                        apps.remove(index);
                        currentApp.setChecked(true);
                        apps.add(index, currentApp);
                    }
                }

            }
            return 1;
        }

        protected void onPostExecute(Integer bill) {
            adapter.notifyDataSetChanged();
        }

    }
*/

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }
}
