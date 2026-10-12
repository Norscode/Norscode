// GENERERT av tools/lag_vscode_klient.no — ikkje rediger for hand.
// Norscode språkklient: startar tenaren (`nc lsp`) over stdio og kopla han til VS Code.
'use strict';
const path = require('path');
const vscode = require('vscode');
const { LanguageClient, TransportKind } = require('vscode-languageclient/node');

let client;

function tenarAlternativ(folder) {
  const cfg = vscode.workspace.getConfiguration('norscode');
  const command = cfg.get('server.command', 'nc');
  const args = cfg.get('server.args', ['lsp']);
  const cwd = folder ? folder.uri.fsPath : undefined;
  const env = Object.assign({}, process.env, cfg.get('server.env', {}));
  if (cwd && !env.NORSCODE_ROOT) env.NORSCODE_ROOT = cwd;
  // Tenaren les filer (moduloppslag) under prosjektrota.
  if (cwd && !env.NORSCODE_VM_DISK_ROOT) env.NORSCODE_VM_DISK_ROOT = cwd + ',/tmp,/private/tmp';
  const run = { command, args, options: { cwd, env }, transport: TransportKind.stdio };
  return { run, debug: run };
}

function activate(context) {
  const folder = vscode.workspace.workspaceFolders && vscode.workspace.workspaceFolders[0];
  const clientOptions = {
    documentSelector: [{ scheme: 'file', language: 'norscode' }, { scheme: 'untitled', language: 'norscode' }],
    synchronize: { fileEvents: vscode.workspace.createFileSystemWatcher('**/*.no') },
    outputChannelName: 'Norscode Language Server',
  };
  client = new LanguageClient('norscode', 'Norscode Language Server', tenarAlternativ(folder), clientOptions);
  context.subscriptions.push(vscode.commands.registerCommand('norscode.restartServer', async () => {
    if (client) await client.stop();
    client = new LanguageClient('norscode', 'Norscode Language Server', tenarAlternativ(folder), clientOptions);
    await client.start();
  }));
  return client.start();
}

function deactivate() {
  return client ? client.stop() : undefined;
}

module.exports = { activate, deactivate };
