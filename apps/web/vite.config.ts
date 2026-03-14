import path from "node:path";
import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import wasm from "vite-plugin-wasm";

const base = process.env.PAGES_BASE_PATH ?? "/";

export default defineConfig({
  base,
  plugins: [react(), wasm()],
  server: {
    fs: {
      allow: [path.resolve(__dirname, "../..")],
    },
  },
});
