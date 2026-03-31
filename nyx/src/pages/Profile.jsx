import React from "react";
import { useUser } from "../context/UserContext";
import { Icon } from "@iconify/react";
import { createClient2, loadWasmFunctions, testHMAC, saveClientKeys } from "../backend/wasmAPI";

const Profile = () => {
    const { keys, setKeys } = useUser();

    const createUser = async () => {
        console.log("Loading WASM...");
        //const wasm = await loadWasmFunctions();

        console.log("Generating keys...");
        /*const newKeys = await createClient(wasm); 
        setKeys({
            upk: newKeys.upk_u_i,
            u_i: newKeys.u_i,
            v_i: newKeys.v_i,
        });*/

        const newKeys = await createClient2(); 
        setKeys({
            upk: newKeys.upk,
            u_i: null,
            v_i: null,
        });

        console.log("Keys generated:", newKeys);

        // Offer download
        const blob = new Blob(
            [
                `User Keys\n\nupk: ${newKeys.upk}\nu_i: ${newKeys.u_i}\nv_i: ${newKeys.v_i}`,
            ],
            { type: "text/plain;charset=utf-8" }
        );
        const url = URL.createObjectURL(blob);
        const a = document.createElement("a");
        a.href = url;
        a.download = "user-keys.txt";
        a.click();
        URL.revokeObjectURL(url);
    };

    // --- Upload Keys ---
    const handleUpload = (event) => {
        const file = event.target.files[0];
        if (!file) return;

        const reader = new FileReader();
        reader.onload = (e) => {
            const text = e.target.result;
            // Parse format: "upk: xxx"
            const upk = text.match(/upk:\s*(.+)/)?.[1];
            const u_i = text.match(/u_i:\s*(.+)/)?.[1];
            const v_i = text.match(/v_i:\s*(.+)/)?.[1];

            if (upk && u_i && v_i) {
                // setKeys({ upk, u_i, v_i });
                saveClientKeys(upk, u_i, v_i);
                setKeys({
                    upk: upk,
                    u_i: null,
                    v_i: null,
                });

                alert("Keys loaded successfully!");
            } else {
                alert("Invalid key file format.");
            }
        };
        reader.readAsText(file);
    };

    // --- HMAC ---
    const handleHMAC = async (event) => {
        const wasm = await loadWasmFunctions();
        testHMAC(wasm, "d47b6ab22bb538a21a08949b405fba9b93f3c86159c7726f64fdb959fc199454", keys.u_i, keys.upk, "Some Message");

    };

    return (
        <div className="profile-page">
            <h1>Profile</h1>

            <div className="buttons-container">
                <button onClick={createUser} className="icon-button">
                    <Icon icon="mdi:account-plus" width="20" height="20" /> Create User
                </button>

                <label className="icon-button">
                    <Icon icon="mdi:file-upload" width="20" height="20" /> Upload Keys
                    <input type="file" accept=".txt" onChange={handleUpload} hidden />
                </label>

                <button onClick={handleHMAC} className="icon-button">
                    <Icon icon="mdi:account-plus" width="20" height="20" /> Test HMAC
                </button>
            </div>

            {keys ? (
                <div className="keys-display">
                    <h2>Current Keys</h2>
                    <p><strong>upk:</strong> {keys.upk}</p>
                    <p><strong>u_i:</strong> {keys.u_i}</p>
                    <p><strong>v_i:</strong> {keys.v_i}</p>
                </div>
            ) : (
                <p>No keys loaded yet.</p>
            )}
        </div>
    );
};

export default Profile;
