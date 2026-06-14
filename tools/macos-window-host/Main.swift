import AppKit
import Foundation
import WebKit

struct HostConfig {
    let repoRoot: String
    let appTemplate: String
    let executable: String
    let initialPath: String
    let initialQuery: String
    let stateDir: String
}

func loadHostConfig() -> HostConfig? {
    guard
        let configURL = Bundle.main.url(forResource: "window-host", withExtension: "env"),
        let raw = try? String(contentsOf: configURL, encoding: .utf8)
    else {
        return nil
    }

    var values: [String: String] = [:]
    for line in raw.split(separator: "\n") {
        let trimmed = line.trimmingCharacters(in: .whitespacesAndNewlines)
        if trimmed.isEmpty || trimmed.hasPrefix("#") {
            continue
        }
        let parts = trimmed.split(separator: "=", maxSplits: 1).map(String.init)
        if parts.count == 2 {
            values[parts[0]] = parts[1]
        }
    }

    guard
        let repoRoot = values["REPO_ROOT"],
        let appTemplate = values["APP_TEMPLATE"],
        let executable = values["NC_EXECUTABLE"]
    else {
        return nil
    }

    return HostConfig(
        repoRoot: repoRoot,
        appTemplate: appTemplate,
        executable: executable,
        initialPath: values["INITIAL_PATH"] ?? "/",
        initialQuery: values["INITIAL_QUERY"] ?? "",
        stateDir: values["STATE_DIR"] ?? NSTemporaryDirectory()
    )
}

func escapeForNorscodeString(_ value: String) -> String {
    value
        .replacingOccurrences(of: "\\", with: "\\\\")
        .replacingOccurrences(of: "\"", with: "\\\"")
        .replacingOccurrences(of: "\n", with: "\\n")
}

func materializeRouteSource(config: HostConfig, path: String, query: String) -> String? {
    guard let template = try? String(contentsOfFile: config.appTemplate, encoding: .utf8) else {
        return nil
    }

    let rendered = template
        .replacingOccurrences(of: "__MAC_WINDOW_PATH__", with: escapeForNorscodeString(path))
        .replacingOccurrences(of: "__MAC_WINDOW_QUERY__", with: escapeForNorscodeString(query))

    let stateDirURL = URL(fileURLWithPath: config.stateDir, isDirectory: true)
    try? FileManager.default.createDirectory(at: stateDirURL, withIntermediateDirectories: true)
    let routeFileURL = stateDirURL.appendingPathComponent("window-route.no")
    do {
        try rendered.write(to: routeFileURL, atomically: true, encoding: .utf8)
        return routeFileURL.path
    } catch {
        return nil
    }
}

func renderHTMLFromNorscode(config: HostConfig, path: String, query: String) -> String? {
    guard let routeFile = materializeRouteSource(config: config, path: path, query: query) else {
        return nil
    }
    let process = Process()
    process.executableURL = URL(fileURLWithPath: config.executable)
    process.arguments = ["run", routeFile]
    process.currentDirectoryURL = URL(fileURLWithPath: config.repoRoot)
    process.environment = [
        "PATH": ProcessInfo.processInfo.environment["PATH"] ?? "",
        "HOME": ProcessInfo.processInfo.environment["HOME"] ?? ""
    ]

    let stdout = Pipe()
    let stderr = Pipe()
    process.standardOutput = stdout
    process.standardError = stderr

    do {
        try process.run()
    } catch {
        return nil
    }

    process.waitUntilExit()

    let stdoutData = stdout.fileHandleForReading.readDataToEndOfFile()
    guard var output = String(data: stdoutData, encoding: .utf8), process.terminationStatus == 0 else {
        return nil
    }

    output = output.replacingOccurrences(of: "\r\n", with: "\n")

    if let htmlRange = output.range(of: "<!doctype html>", options: [.caseInsensitive]) {
        return String(output[htmlRange.lowerBound...])
    }

    if let htmlRange = output.range(of: "<html", options: [.caseInsensitive]) {
        return String(output[htmlRange.lowerBound...])
    }

    return nil
}

func rewriteAssetPaths(_ html: String, repoRoot: String) -> String {
    let cssPath = "file://\(repoRoot)/frontend/assets/css/app.css"
    return html.replacingOccurrences(of: "href=\"/assets/css/app.css\"", with: "href=\"\(cssPath)\"")
}

func fallbackHTML(_ message: String) -> String {
    """
    <!doctype html>
    <html lang="nb">
      <head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <title>Norscode Window Host</title>
        <style>
          :root {
            color-scheme: light dark;
            --bg-a: #f6efe3;
            --bg-b: #dce8f3;
            --panel: rgba(255,255,255,0.76);
            --ink: #17212b;
            --muted: #5f6b76;
            --accent: #0a6b5c;
            --line: rgba(23, 33, 43, 0.12);
          }
          @media (prefers-color-scheme: dark) {
            :root {
              --bg-a: #162028;
              --bg-b: #0d1117;
              --panel: rgba(17, 24, 32, 0.78);
              --ink: #edf3f8;
              --muted: #a6b3bf;
              --accent: #67d8be;
              --line: rgba(237, 243, 248, 0.12);
            }
          }
          * { box-sizing: border-box; }
          body {
            margin: 0;
            min-height: 100vh;
            font-family: "SF Pro Display", "Helvetica Neue", sans-serif;
            color: var(--ink);
            background:
              radial-gradient(circle at top left, var(--bg-a), transparent 45%),
              linear-gradient(135deg, var(--bg-b), #f9fbfd 140%);
            display: grid;
            place-items: center;
            padding: 28px;
          }
          .card {
            width: min(760px, 100%);
            backdrop-filter: blur(18px);
            background: var(--panel);
            border: 1px solid var(--line);
            border-radius: 28px;
            padding: 34px;
            box-shadow: 0 18px 60px rgba(0,0,0,0.10);
          }
          .eyebrow {
            font-size: 12px;
            font-weight: 700;
            letter-spacing: 0.14em;
            text-transform: uppercase;
            color: var(--accent);
            margin: 0 0 14px;
          }
          h1 {
            margin: 0 0 14px;
            font-size: clamp(34px, 5vw, 52px);
            line-height: 0.96;
          }
          p {
            margin: 0 0 16px;
            font-size: 18px;
            line-height: 1.55;
            color: var(--muted);
          }
          code {
            font-family: "SF Mono", "Menlo", monospace;
            font-size: 0.95em;
          }
        </style>
      </head>
      <body>
        <main class="card">
          <p class="eyebrow">Omgang 3</p>
          <h1>Norscode-hosten fann ikkje HTML-endepunktet.</h1>
          <p>\(message)</p>
          <p>
            Verten er likevel bygd for å hente innhald frå Norscode-køyring,
            ikkje frå hardkoda Swift-markup.
          </p>
        </main>
      </body>
    </html>
    """
}

final class AppDelegate: NSObject, NSApplicationDelegate {
    private var controller: WindowController?

    func applicationDidFinishLaunching(_ notification: Notification) {
        controller = WindowController()
        controller?.showWindow(nil)
        NSApp.activate(ignoringOtherApps: true)
    }

    @objc func reloadContent(_ sender: Any?) {
        controller?.reloadCurrentRoute()
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        true
    }
}

final class WindowController: NSWindowController, WKNavigationDelegate {
    private var hostConfig: HostConfig?
    private var webView: WKWebView?
    private var currentPath = "/"
    private var currentQuery = ""

    convenience init() {
        let contentRect = NSRect(x: 0, y: 0, width: 980, height: 680)
        let styleMask: NSWindow.StyleMask = [
            .titled,
            .closable,
            .miniaturizable,
            .resizable
        ]

        let window = NSWindow(
            contentRect: contentRect,
            styleMask: styleMask,
            backing: .buffered,
            defer: false
        )
        window.title = "Norscode"
        window.center()
        window.isReleasedWhenClosed = false

        let config = WKWebViewConfiguration()
        let webView = WKWebView(frame: .zero, configuration: config)
        webView.autoresizingMask = [.width, .height]
        webView.setValue(false, forKey: "drawsBackground")
        webView.navigationDelegate = nil
        window.contentView = webView
        self.init(window: window)
        self.webView = webView
        self.hostConfig = loadHostConfig()
        self.currentPath = hostConfig?.initialPath ?? "/"
        self.currentQuery = hostConfig?.initialQuery ?? ""
        webView.navigationDelegate = self
        renderCurrentRoute()
    }

    func renderCurrentRoute() {
        guard let webView, let config = hostConfig else {
            webView?.loadHTMLString(fallbackHTML("Manglar bundle-konfig for Norscode vindushost."), baseURL: nil)
            return
        }
        let rawHTML = renderHTMLFromNorscode(config: config, path: currentPath, query: currentQuery)
            ?? fallbackHTML("Klarte ikkje å hente HTML frå \(config.executable) run <materialisert rute> for \(currentPath).")
        let html = rewriteAssetPaths(rawHTML, repoRoot: config.repoRoot)
        window?.title = currentQuery.isEmpty ? "Norscode - \(currentPath)" : "Norscode - \(currentPath)?\(currentQuery)"
        webView.loadHTMLString(html, baseURL: URL(string: "https://norscode.app"))
    }

    func reloadCurrentRoute() {
        renderCurrentRoute()
    }

    func navigate(to url: URL) {
        currentPath = url.path.isEmpty ? "/" : url.path
        currentQuery = url.query ?? ""
        renderCurrentRoute()
    }

    func webView(_ webView: WKWebView, decidePolicyFor navigationAction: WKNavigationAction, decisionHandler: @escaping (WKNavigationActionPolicy) -> Void) {
        guard navigationAction.navigationType != .other, let url = navigationAction.request.url else {
            decisionHandler(.allow)
            return
        }
        if url.scheme == "https", url.host == "norscode.app" {
            navigate(to: url)
            decisionHandler(.cancel)
            return
        }
        decisionHandler(.allow)
    }

    override func keyDown(with event: NSEvent) {
        if event.modifierFlags.contains(.command), event.charactersIgnoringModifiers?.lowercased() == "r" {
            reloadCurrentRoute()
            return
        }
        super.keyDown(with: event)
    }
}

let app = NSApplication.shared
let delegate = AppDelegate()
app.setActivationPolicy(.regular)
app.delegate = delegate
let menubar = NSMenu()
let appMenuItem = NSMenuItem()
menubar.addItem(appMenuItem)
app.mainMenu = menubar
let appMenu = NSMenu()
appMenuItem.submenu = appMenu
appMenu.addItem(withTitle: "Last inn på nytt", action: #selector(AppDelegate.reloadContent(_:)), keyEquivalent: "r")
appMenu.addItem(NSMenuItem.separator())
appMenu.addItem(withTitle: "Avslutt Norscode", action: #selector(NSApplication.terminate(_:)), keyEquivalent: "q")
app.run()
