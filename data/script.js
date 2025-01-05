// Hàm fetch dữ liệu cảm biến từ ESP32
async function fetchData() {
    try {
        const response = await fetch("/sensor");
        if (!response.ok) throw new Error("Failed to fetch sensor data.");

        const data = await response.json();

        // Cập nhật dữ liệu cảm biến lên giao diện
        document.getElementById("temperature").textContent = data.temperature.toFixed(2);
        document.getElementById("humidity").textContent = data.humidity.toFixed(2);
        document.getElementById("light").textContent = data.light;
        document.getElementById("moisture").textContent = data.moisture;
    } catch (error) {
        console.error("Error fetching data:", error);
    }
}

// Hàm gửi lệnh bật/tắt LED
async function toggleLED() {
    try {
        const button = document.getElementById("led-button");
        const newState = button.textContent.includes("OFF") ? "on" : "off";

        const response = await fetch(`/led/${newState}`, { method: "POST" });
        if (!response.ok) throw new Error("Failed to toggle LED.");

        button.textContent = `LED: ${newState.toUpperCase()}`;
        button.classList.toggle("off", newState === "off");
    } catch (error) {
        console.error("Error toggling LED:", error);
    }
}

// Hàm gửi lệnh bật/tắt máy bơm
async function togglePump() {
    try {
        const button = document.getElementById("pump-button");
        const newState = button.textContent.includes("OFF") ? "on" : "off";

        const response = await fetch(`/pump/${newState}`, { method: "POST" });
        if (!response.ok) throw new Error("Failed to toggle pump.");

        button.textContent = `Pump: ${newState.toUpperCase()}`;
        button.classList.toggle("off", newState === "off");
    } catch (error) {
        console.error("Error toggling pump:", error);
    }
}

// Gắn sự kiện click cho các nút điều khiển
document.getElementById("led-button").addEventListener("click", toggleLED);
document.getElementById("pump-button").addEventListener("click", togglePump);

// Lấy dữ liệu cảm biến định kỳ mỗi 2 giây
setInterval(fetchData, 2000);

// Lấy dữ liệu lần đầu khi trang tải
fetchData();
