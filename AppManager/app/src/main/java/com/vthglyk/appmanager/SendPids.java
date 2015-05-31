package com.vthglyk.appmanager;

import android.app.Activity;
import android.content.Intent;
import android.support.v7.app.ActionBarActivity;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.TextView;


public class SendPids extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        /* Get the message from the intent */
        Intent intent = getIntent();
        //String message = intent.getStringExtra(MainActivity.EXTRA_MESSAGE);
        int[] H_table = intent.getIntArrayExtra("pids");
        int H_table_dim = intent.getIntExtra("H_table_dim", 100);

        /* Create a TextView Object */
        TextView textView = new TextView(this);
        textView.setTextSize(20);
        textView.setText(send_Pids_to_kernel(H_table_dim, H_table));

        /* Set the text view as the activity layout */
        setContentView(textView);
    }


    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_send_pids, menu);
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

    public native String send_Pids_to_kernel(int H_table_dim, int[] H_table);
    static {
        System.loadLibrary("send_Pids_to_kernel");
    }

}
