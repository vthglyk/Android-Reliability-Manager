package com.vthglyk.appmanager;

import android.app.Activity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;

/**
 * Created by lebro_000 on 13-Mar-15.
 */
public class AppManagerAdapter extends ArrayAdapter<App> {

    private final ArrayList<App> list;
    private final Activity context;
    private final int resource;

    public AppManagerAdapter(Activity context, int resource, ArrayList<App> list) {
        super(context, resource, list);
        this.context = context;
        this.list = list;
        this.resource = resource;

    }

    static class ViewHolder {
        protected TextView name;
        protected TextView pid;
        protected TextView uid;
        protected CheckBox checkbox;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {

        View view = null;

        if (convertView == null) {
            LayoutInflater myInflater = context.getLayoutInflater();
            view = myInflater.inflate(resource, null);
            final ViewHolder viewHolder = new ViewHolder();
            viewHolder.name = (TextView) view.findViewById(R.id.app_name);
            viewHolder.pid = (TextView) view.findViewById(R.id.app_pid);
            viewHolder.uid = (TextView) view.findViewById(R.id.app_uid);
            viewHolder.checkbox = (CheckBox) view.findViewById(R.id.priority_checkbox);
            viewHolder.checkbox
                    .setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {

                        @Override
                        public void onCheckedChanged(CompoundButton buttonView,
                                                     boolean isChecked) {

                            App element = (App) viewHolder.checkbox.getTag();
                            element.setChecked(buttonView.isChecked());

                            String str1;
                            String str2;
                            if(element.getChecked()) {
                                str1 = "You selected ";
                                str2 = " as high priority application";
                            }
                            else {
                                str1 = "You removed ";
                                str2 = " from the high priority applications";
                            }

                            /*Toast.makeText(context,
                                    str1 + element.getName() + str2,
                                    Toast.LENGTH_SHORT).show();*/
                        }
                    });
            view.setTag(viewHolder);
            viewHolder.checkbox.setTag(list.get(position));
        } else {
            view = convertView;
            ((ViewHolder) view.getTag()).checkbox.setTag(list.get(position));
        }
        ViewHolder holder = (ViewHolder) view.getTag();
        App curr_app = list.get(position);
        holder.name.setText(curr_app.getName());
        holder.pid.setText(String.valueOf(curr_app.getPid()));
        holder.uid.setText(String.valueOf(curr_app.getUid()));
        holder.checkbox.setChecked(curr_app.getChecked());
        return view;
    }
}
