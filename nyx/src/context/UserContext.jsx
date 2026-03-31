// src/context/UserContext.jsx
import React, { createContext, useState, useContext } from "react";

const UserContext = createContext();

export const UserProvider = ({ children }) => {
  const [keys, setKeys] = useState(null); // { upk, u_i, v_i }

  return (
    <UserContext.Provider value={{ keys, setKeys }}>
      {children}
    </UserContext.Provider>
  );
};

export const useUser = () => useContext(UserContext);
