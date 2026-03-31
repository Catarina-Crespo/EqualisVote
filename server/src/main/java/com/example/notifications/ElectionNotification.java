package com.example.notifications;

import com.example.models.Election;

import java.util.UUID;

import static com.example.Utils.ELECTION_INVITE;

public class ElectionNotification implements Notification {

    private String type = ELECTION_INVITE;
    private String ID;
    public String epk_E;
    public String name;
    public String[] options;
    public String deadline;
    public String groupId;


    public ElectionNotification(Election election) {
        this.ID = UUID.randomUUID().toString();
        this.epk_E = election.getEpk_E();
        this.name = election.getName();
        this.options = election.getOptions();
        this.deadline = election.getDeadline();
        this.groupId = election.getGroup_spk();
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
