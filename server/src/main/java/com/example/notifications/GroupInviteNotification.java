package com.example.notifications;

import java.util.List;
import java.util.UUID;

import static com.example.Utils.GROUP_INVITE;

public class GroupInviteNotification implements Notification {
    public String type = GROUP_INVITE;
    public String ID;
    public List<String> groupMembers;
    public String groupId;
    public String name;

    public GroupInviteNotification(List<String> groupMembers, String groupId, String groupName) {
        this.groupMembers = groupMembers;
        this.groupId = groupId;
        this.ID = UUID.randomUUID().toString();
        this.name = groupName;
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
