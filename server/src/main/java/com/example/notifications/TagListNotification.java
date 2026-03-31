package com.example.notifications;

import java.util.UUID;

public class TagListNotification implements Notification {
    public String type;
    public String ID;
    public String tagList;
    public String upk;
    public int groupSize;
    public String spk;

    public TagListNotification(String type, String tagList, String userUpk, int groupSize, String spk) {
        this.type = type;
        this.tagList = tagList;
        this.upk = userUpk;
        this.groupSize = groupSize;
        this.spk = spk;
        this.ID = UUID.randomUUID().toString();
    }

    @Override
    public String getType() {
        return this.type;
    }

    @Override
    public String getID() {
        return this.ID;
    }
}
