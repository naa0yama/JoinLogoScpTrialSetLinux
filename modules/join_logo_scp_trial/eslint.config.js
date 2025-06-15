// @ts-check

import eslint from '@eslint/js';
import globals from 'globals';

export default [
  {
    ignores: [
      "node_modules/",
      "dist/",
      "coverage/",
      "eslint.config.js"
    ]
  },
  eslint.configs.recommended,
  {
    languageOptions: {
      ecmaVersion: 2022,
      sourceType: 'module',
      globals: {
        ...globals.node,
        // Add any other global variables your project uses
      }
    },
    rules: {
      // Add any custom rules or overrides here
      "no-unused-vars": "warn",
      "no-console": "off",
    }
  }
];
