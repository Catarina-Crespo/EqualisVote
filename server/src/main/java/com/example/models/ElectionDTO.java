package com.example.models;

public class ElectionDTO {

    public String epk_E;
    public String name;
    public String[] options;
    public String deadline;
    public String group_spk;
    public boolean done;

    public ElectionDTO(Election election) {
        this.epk_E = election.getEpk_E();
        this.name = election.getName();
        this.options = election.getOptions();
        this.deadline = election.getDeadline();
        this.group_spk = election.getGroup_spk();
        this.done = election.isDone();
    }
}
