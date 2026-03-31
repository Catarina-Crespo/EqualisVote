package com.example.models;

import java.util.LinkedList;
import java.util.List;

public class Group {

    public String spk;
    private String s_G;
    public String groupName;
    private final List<User> users;
    private final List<String> tags;
    private final List<String> invitedUsers;
    private final List<Election> elections;

    public Group(String spk, String s_G) {
        this.spk = spk;
        this.s_G = s_G;
        this.groupName = null;
        this.users = new LinkedList<>();
        this.tags = new LinkedList<>();
        this.invitedUsers = new LinkedList<>();
        this.elections = new LinkedList<>();
    }

    public String getSpk() {
        return spk;
    }

    public void setSpk(String spk) {
        this.spk = spk;
    }

    public void setGroupName(String name) {
        this.groupName = name;
    }

    public String getGroupName() {
        return groupName;
    }

    public String getS_G() {
        return s_G;
    }

    public void setS_G(String s_G) {
        this.s_G = s_G;
    }

    public List<String> getTags() {
        return tags;
    }

    public List<User> getUsers() {
        return users;
    }

    public List<String> getUsersUpks() {
        List<String> result = new LinkedList<>();
        for (User u : users)
            result.add(u.getUpk());

        return result;
    }

    public void addUser(User user, String tag) {
        if (!users.contains(user)) {
            users.add(user);
            tags.add(tag);
        }

        invitedUsers.remove(user.getUpk());
    }

    public void removeUser(User user, String tag) {
        users.remove(user);
        tags.remove(tag);
    }

    public void addInvitedUser(String upk) {
        if (!invitedUsers.contains(upk)) {
            invitedUsers.add(upk);
        }
    }

    public boolean isUserInvited(String upk) {
        return invitedUsers.contains(upk);
    }

    public void removeInvitedUser(String upk) {
        invitedUsers.remove(upk);
    }

    public void addBulkInvites(List<String> upks) {
        invitedUsers.addAll(upks);
    }

    public void addElection(Election election) {
        elections.add(election);
    }

    public List<Election> getElections() {
        return elections;
    }

    public List<String> getInvitedUsers() {
        return invitedUsers;
    }

}
