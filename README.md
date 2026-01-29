<h1 align="center">💬 Multilingual Chat Server – Network & Localization Demo</h1>
<p align="center">
  This project was developed to gain <strong>hands-on experience</strong> with <strong>C++ network programming</strong>
  and to deepen understanding of <strong>multithreaded server design</strong> and <strong>multilingual text handling</strong>.<br>
  A real-time chat environment was chosen to directly address concurrency control,
  packet reliability, and UTF-8–based localization challenges.
</p>

<p align="center">
  <a href="./README.md">English</a> |
  <a href="./README_jp.md">日本語</a>
</p>


## 🌟 Highlights
<table>
  <tr>
    <td style="text-align:center">
      <img src="https://github.com/user-attachments/assets/680fa7b4-2c07-45b0-80b2-10107a378926" width="400" height="300">
    </td>
    <td style="text-align:center">
      <img src="https://github.com/user-attachments/assets/e903cfc3-2c40-4f90-adee-240cd365009c" width="400" height="300">
    </td>
  </tr>
</table>
<br>

## 🔗 Link
- [Demo Video](https://youtu.be/K_OWQfoCe0c)

<br>

## 📜 Project Overview
- **Duration**: `2025.09.07 ~ 2025.09.14`
- **Team Size**: `1 member(solo development)`
- **Key Focus**: `Multithreading`, `Networking`, `Packet Design`, `Localization`

<br>

## ⚙️ Development Environment
- **Language**: `C++17`
- **Network API**: `WinSock2`
- **Tools**: `Visual Studio 2024`

<br>

## ⚒️ Implementations
- **Server Architecture**: Thread pool–based multi-threaded server with synchronized shared resources.
- **Client Structure**: Separate send/receive threads to prevent blocking during input and message reception.
- **Packet Handling**: Header-based packet protocol with explicit serialization and deserialization.
- **Text Processing**: Automatic line wrapping and client-side prohibited word filtering.

<br>

## ⚠️ Notes
- This project was developed for **educational purposes** as a **personal technical exercise**.
- The implementation focuses on **blocking I/O–based design**, with future improvements planned for
  **asynchronous networking (e.g., IOCP) and performance optimization**.
