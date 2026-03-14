import path from "node:path";
import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import wasm from "vite-plugin-wasm";

export default defineConfig({
  plugins: [react(), wasm()],
  server: {
    fs: {
      allow: [path.resolve(__dirname, "../..")],
    },
  },
});
