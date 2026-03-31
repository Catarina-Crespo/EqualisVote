import React, { useState, useEffect } from 'react';
import axios from 'axios';
import { Icon } from "@iconify/react";
import Notification from "../components/Notification";
import { getNotifications } from '../backend/wasmAPI';
import { useUser } from '../context/UserContext';

const Notifications = () => {
  const [notifications, setNotifications] = useState([]);

  const { keys } = useUser();

  const handleRemove = (id) => {
    setNotifications((prev) => prev.filter((notif) => notif.ID !== id));
  };

  let dummyNotifictions = [
    {
      id: 1,
      type: "group_invite",
      data: { groupName: "Engineering Team", groupId: "5a35e32ca9b9cfe2da9afb09ef371b6fedfa286c3ced3d842142a2d4dea18468" }
    },
    {
      id: 2,
      type: "group_invite",
      data: { groupName: "Marketing Team", groupId: "e294f57b47782e13b83d4ce10a8f5b4e5bbb06e84caed5aa7f9c9c0996caff74" }
    },
    {
      id: 3,
      type: "election_invite",
      data: { groupName: "HR Team", groupId: "12d74f8624b6b3333a819bdbc8e5e46765163ae0c51e1d812b49e5104ed00b59" }
    }
  ];

  const handleRefresh = () => {
    fetchNotifications();
  }

  const fetchNotifications = async () => {
    if (!keys) return;

    try {
      const result = await getNotifications(keys.upk);

      if (result.size > 0 && result.notifications) {
        const normalized = result.notifications.map((notifStr) => {
          let notif = notifStr;
          if (typeof notifStr === "string") {
            try {
              notif = JSON.parse(notifStr);
            } catch (err) {
              console.error("Invalid JSON:", notifStr, err);
            }
          }

          let data = notif.data || {};
          if (!notif.data) {
            Object.keys(notif).forEach((k) => {
              if (k !== "type") data[k] = notif[k];
            });
          }

          return {
            id: notif.ID,
            type: notif.type,
            data,
          };
        }).filter(Boolean);

        setNotifications(normalized);
      } else {
        setNotifications([]);
      }

      console.log("Fetched notifications:", result.size);
    } catch (error) {
      console.error("Error fetching notifications:", error);
    }
  };

  // remove from UI immediately
  const removeNotification = (id) => {
    setNotifications((prev) => prev.filter((n) => n.id !== id));
  };


  useEffect(() => {
    fetchNotifications();
  }, [keys]); // re-run when keys change

  console.log("Notifs received: ")
  console.log(notifications)


  return (
    <div className="notifications-section">
      <div className='header'>
        <h1>Notifications</h1>
        <button onClick={handleRefresh} name='refresh' data-testid="refresh"><Icon icon="material-symbols:refresh-rounded" width="32" height="32" /></button>
      </div>
      {notifications.length === 0 ? (
        <p>No notifications.</p>
      ) : (
        <ul>
          {notifications.map(notif => (
            <li key={notif.id}><Notification isGroupInvite={notif.type} data={notif.data} refresh={fetchNotifications} onRemove={removeNotification} /></li>
          ))}

        </ul>
      )}
    </div>
  );
};

export default Notifications;
