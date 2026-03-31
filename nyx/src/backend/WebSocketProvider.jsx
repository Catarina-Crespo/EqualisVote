import React, { createContext, useEffect, useRef, useState } from "react";
import { useUser } from "../context/UserContext";
import { loadWasmFunctions } from "./wasmAPI";

export const WebSocketContext = createContext(null);

export const WebSocketProvider = ({ children }) => {
  const [socket, setSocket] = useState(null);
  const [alpha, setAlpha] = useState(null);
  const [Tpp, setTpp] = useState(null);
  const reconnectAttempts = useRef(0);
  const reconnectTimeout = useRef(null);

  let alphaTemp = '';
  let TppTemp = '';

  const { keys } = useUser();

  const connect = async (key) => {
    const ws = new WebSocket(`ws://localhost:8080/server_war/ws/${key}`);

    const wasm = await loadWasmFunctions();

    setSocket(ws);

    ws.onopen = () => {
      console.log("WebSocket connected with key:", key);
      reconnectAttempts.current = 0;
    };

    ws.onmessage = async (data) => {

      //console.log("Logging WS: ")

      //console.log(data.data);
      const msg = JSON.parse(data.data);

      console.log("WS Received:", msg);
      
      switch (msg.type) {
        case "int_step_1_C": {
          const result = JSON.parse(
            await wasm.groupInitExp(msg.groupSize, msg.tagList)
          );
        
          console.log("Alpha produced: " + result.alpha)
          alphaTemp = result.alpha;
          setAlpha(alphaTemp);
          //alpha = result.alpha; - TODO - I guess I should make it a variable
          console.log("Set alpha: " + alphaTemp)

          ws.send(
            JSON.stringify({
              type: "int_step_1_reply_C",
              T: result.T,
              upk: msg.upk,
              group_size: msg.groupSize,
              spk: msg.spk,
            })
          );
          break;
        }
        case "int_step_1": {
          const result = JSON.parse(
            wasm.groupInitExp(msg.groupSize, msg.tagList)
          );
          
          console.log("Alpha produced: " + result.alpha)
          alphaTemp = result.alpha;
          setAlpha(alphaTemp);
          //alpha = result.alpha;

          ws.send(
            JSON.stringify({
              type: "int_step_1_reply",
              T: result.T,
              upk: msg.upk,
              group_size: msg.groupSize,
              spk: msg.spk,
            })
          );
          break;
        }
        case "int_step_2": {
          const result = JSON.parse(
            wasm.shuffleUser(keys.u_i, keys.upk, msg.Tprime)
          );

          setTpp(result.Tpp);
          //Tpp = result.Tpp;
          ws.send(
            JSON.stringify({
              type: "int_step_2_reply",
              Tpp: result.Tpp,
              upk: msg.upk,
              group_size: msg.groupSize,
              spk: msg.spk,
            })
          );
          break;
        }
        case "int_step_3": {

          
          const result = JSON.parse(
            wasm.intersection(
              msg.groupSize,
              alphaTemp,
              msg.Ws,
              msg.overline_spk_i_start_G,
              msg.Tpp,
              msg.minVote,
              msg.maxVote,
              msg.nBallots
            )
          );

          ws.send(
            JSON.stringify({
              type: "int_step_3_reply",
              X: result.X,
              upk: msg.upk,
              spk: msg.spk,
            })
          );
          break;
        }
        case "int_step_3_C": {
          console.log("Logging alpha: " + alphaTemp);
          console.log("Logging Tpp: " + (TppTemp == msg.Tpp)) 
          console.log("Logging TppTemp " + TppTemp);
          console.log("Logging msg.Tpp " + msg.Tpp);
          
          const result = JSON.parse(
            wasm.intersection(
              msg.groupSize,
              alphaTemp,
              msg.Ws,
              msg.overline_spk_i_start_G,
              msg.Tpp,
              msg.minVote,
              msg.maxVote,
              msg.nBallots
            )
          );

          console.log("Logging final result: ")
          console.log(result);

          ws.send(
            JSON.stringify({
              type: "int_step_3_reply_C",
              X: result.X,
              upk: msg.upk,
              spk: msg.spk,
            })
          );
          break;
        }

        case "int_step_4": {
          const result = JSON.parse(
            wasm.generateTag(keys.upk, keys.v_i, msg.spk)
          );
          ws.send(
            JSON.stringify({
              type: "int_step_4_reply",
              tag: result.z_i_G,
              upk: msg.upk,
              spk: msg.spk,
            })
          );
          break;
        }
        case "int_step_5": {
          console.log(msg.message);
          break;
        }
        default:
          console.warn("Unhandled WS msg:", msg.type);
      }


    };

    ws.onclose = (event) => {
      console.warn("WebSocket closed", event.reason);
      scheduleReconnect(key);
    };

    ws.onerror = (err) => {
      console.error("WebSocket error:", err.message);
      ws.close();
    };
  };

  const scheduleReconnect = (key) => {
    if (reconnectTimeout.current) return; // already scheduled

    const delay = Math.min(1000 * 2 ** reconnectAttempts.current, 30000);
    reconnectAttempts.current += 1;

    console.log(`Attempting reconnect in ${delay / 1000}s...`);

    reconnectTimeout.current = setTimeout(() => {
      reconnectTimeout.current = null;
      connect(key);
    }, delay);
  };

  useEffect(() => {
    if (!keys) return;      // Don't connect if keys don't exist

    const key = keys.upk;   // Use uploaded keys
    connect(key);

    return () => {
      if (reconnectTimeout.current) {
        clearTimeout(reconnectTimeout.current);
      }
      if (socket) {
        socket.close();
      }
    };
  }, [keys]);               // Re-run whenever keys change

  return (
    <WebSocketContext.Provider value={socket}>
      {children}
    </WebSocketContext.Provider>
  );
};
