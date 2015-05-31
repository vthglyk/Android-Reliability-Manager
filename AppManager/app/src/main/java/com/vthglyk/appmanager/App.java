package com.vthglyk.appmanager;

/**
 * Created by lebro_000 on 13-Mar-15.
 */
public class App {

    private String name;
    private int pid;
    private int uid;
    private boolean checked;

    public App(String name, int pid, int uid) {
        this.name = name;
        this.pid = pid;
        this.uid = uid;
        this.checked = false;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public int getPid() {
        return pid;
    }

    public void setPid(int pid) {
        this.pid = pid;
    }

    public int getUid() {
        return uid;
    }

    public void setUid(int pid) {
        this.pid = uid;
    }

    public boolean getChecked() {
        return checked;
    }

    public void setChecked(boolean checked) {
        this.checked = checked;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == null) {
            return false;
        }

        if (getClass() != obj.getClass()) {
            return false;
        }

        final App other = (App) obj;
        if (!this.name.equals(other.name) ) {
            return false;
        }

        if(this.pid != other.pid) {
            return false;
        }
        return true;
    }

}
