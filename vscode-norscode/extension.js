'use strict';

const cp = require('child_process');
const path = require('path');
const vscode = require('vscode');


function escapeForOutput(value) {
  return String(value).replace(/\r?\n/g, ' ');
}

function getActiveNorscodeDocument() {
  const editor = vscode.window.activeTextEditor;
  if (!editor) {
    return null;
  }

  const document = editor.document;
  if (document.languageId !== 'norscode' && path.extname(document.fileName).toLowerCase() !== '.no') {
    return null;
  }

  return document;
}

async function ensureSaved(document) {
  if (document.isUntitled || document.isDirty) {
    const saved = await document.save();
    if (!saved) {
      throw new Error('Lagring av aktiv fil ble avbrutt.');
    }
  }

  if (!document.fileName) {
    throw new Error('Filen må lagres på disk før den kan kjøres.');
  }
}

function runBridgeCommand(mode, outputLabel, successMessage, failureMessage) {
  const output = vscode.window.createOutputChannel('Norscode');
  const document = getActiveNorscodeDocument();

  if (!document) {
    vscode.window.showWarningMessage(`Åpne en Norscode-.no-fil for å ${outputLabel} den aktive fila.`);
    return;
  }

  ensureSaved(document)
    .then(() => {
      const config = vscode.workspace.getConfiguration('norscode');
      const norscodePath = config.get('norscodePath', 'norscode').trim() || 'norscode';
      const filePath = document.fileName;
      const cwd = path.dirname(filePath);
      const bridgePath = path.join(__dirname, 'bridge.no');

      output.clear();
      output.show(true);
      output.appendLine(`Norscode: ${outputLabel} aktiv fil`);
      output.appendLine(`Fil: ${escapeForOutput(filePath)}`);
      output.appendLine(`Norscode-bridge: ${escapeForOutput(bridgePath)}`);
      output.appendLine(`Kommando: ${escapeForOutput(norscodePath)} run ${escapeForOutput(bridgePath)} ${escapeForOutput(mode)} ${escapeForOutput(filePath)} ${escapeForOutput(norscodePath)}`);
      output.appendLine('');

      const child = cp.spawn(norscodePath, ['run', bridgePath, mode, filePath, norscodePath], {
        cwd,
        env: process.env,
        shell: false,
      });

      child.stdout.on('data', (chunk) => {
        output.append(chunk.toString());
      });

      child.stderr.on('data', (chunk) => {
        output.append(chunk.toString());
      });

      child.on('error', (err) => {
        output.appendLine('');
        output.appendLine(`Kunne ikke starte '${norscodePath}': ${err.message}`);
        vscode.window.showErrorMessage(`Kunne ikke starte '${norscodePath}'. Se Norscode-output for detaljer.`);
      });

      child.on('close', (code) => {
        output.appendLine('');
        output.appendLine(`Prosess avsluttet med kode ${code}`);
        if (code === 0) {
          vscode.window.showInformationMessage(successMessage);
        } else {
          vscode.window.showErrorMessage(`${failureMessage} Kode ${code}. Se Norscode-output for detaljer.`);
        }
      });
    })
    .catch((err) => {
      output.appendLine(`Feil: ${err.message}`);
      output.show(true);
      vscode.window.showErrorMessage(err.message);
    });
}

function runCurrentFile() {
  runBridgeCommand('run', 'kjører', 'Norscode-fila ble kjørt.', 'Norscode avsluttet med feil.');
}

function checkCurrentFile() {
  runBridgeCommand('check', 'sjekker', 'Norscode-fila ble sjekket uten feil.', 'Norscode check avsluttet med feil.');
}

function activate(context) {
  const disposable = vscode.commands.registerCommand('norscode.runCurrentFile', runCurrentFile);
  const checkDisposable = vscode.commands.registerCommand('norscode.checkCurrentFile', checkCurrentFile);
  context.subscriptions.push(disposable, checkDisposable);
}

function deactivate() {}

module.exports = {
  activate,
  deactivate,
};
