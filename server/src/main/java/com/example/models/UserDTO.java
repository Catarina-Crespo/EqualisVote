package com.example.models;

import java.util.LinkedList;
import java.util.List;

public class UserDTO {

    public String upk;
    public String name;
    private final List<Group> groups; // TODO: Change to DTO

    public UserDTO(String upk, String name) {
        this.upk = upk;
        this.name = name;
        this.groups = new LinkedList<>();
    }

    public UserDTO(User user) {
        this.upk = user.getUpk();
        this.name = user.getUsername();
        this.groups = user.getGroups();
    }

}
