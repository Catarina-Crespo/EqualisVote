import React, { useEffect, useState, createContext } from "react";
import { BrowserRouter as Router, Route, Routes } from 'react-router-dom';
import Sidebar from './components/Sidebar';
import Home from './pages/Home';
import Groups from './pages/Groups';
import Notifications from './pages/Notifications';
import Elections from './pages/Elections';
import GroupPage from './pages/GroupPage';
import ElectionPage from './pages/ElectionPage';
import Profile from './pages/Profile';
import { WebSocketProvider } from "./backend/WebSocketProvider";

import './App.css';


const App = () => {

  return (
    <WebSocketProvider>
      <Router>
        <div className="container">
          <Sidebar />

          <main className="main-content">
            <Routes>
              <Route path="/" element={<Home />} />
              <Route path="/groups" element={<Groups />} />
              <Route path="/notifications" element={<Notifications />} />
              <Route path="/elections" element={<Elections />} />
              <Route path="/elections/:epk" element={<ElectionPage />} />
              <Route path="/groups/:spk" element={<GroupPage />} />
              <Route path="/profile" element={<Profile />} />

            </Routes>
          </main>
        </div>
      </Router>
    </WebSocketProvider>
  );
};

export default App;
