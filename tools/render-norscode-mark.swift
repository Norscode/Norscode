import Foundation
import CoreGraphics
import ImageIO
import UniformTypeIdentifiers

struct Config {
    var output: URL?
    var size: Int = 1024
}

func fail(_ message: String) -> Never {
    fputs("Feil: \(message)\n", stderr)
    exit(1)
}

func parseConfig() -> Config {
    var config = Config()
    var index = 1
    while index < CommandLine.arguments.count {
        let argument = CommandLine.arguments[index]
        switch argument {
        case "--output":
            index += 1
            guard index < CommandLine.arguments.count else {
                fail("manglar verdi for --output")
            }
            config.output = URL(fileURLWithPath: CommandLine.arguments[index])
        case "--size":
            index += 1
            guard index < CommandLine.arguments.count, let parsed = Int(CommandLine.arguments[index]), parsed > 0 else {
                fail("manglar gyldig verdi for --size")
            }
            config.size = parsed
        case "-h", "--help":
            print("Bruk: swift render-norscode-mark.swift --output FIL.png [--size N]")
            exit(0)
        default:
            fail("ukjent argument: \(argument)")
        }
        index += 1
    }
guard config.output != nil else {
    fail("manglar --output")
}
return config
}

let config = parseConfig()
let size = config.size
let output = config.output!

func renderImage(size: Int) -> CGImage {
    let scale = CGFloat(size) / 512.0
    let colorSpace = CGColorSpaceCreateDeviceRGB()
    let context = CGContext(
        data: nil,
        width: size,
        height: size,
        bitsPerComponent: 8,
        bytesPerRow: size * 4,
        space: colorSpace,
        bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue
    )!

    context.setAllowsAntialiasing(true)
    context.setShouldAntialias(true)
    context.interpolationQuality = .high
    context.translateBy(x: 0, y: CGFloat(size))
    context.scaleBy(x: scale, y: -scale)

    func c(_ r: CGFloat, _ g: CGFloat, _ b: CGFloat, _ a: CGFloat = 1.0) -> CGColor {
        CGColor(red: r, green: g, blue: b, alpha: a)
    }

    let bg = CGColor(red: 0.01, green: 0.02, blue: 0.05, alpha: 1.0)
    context.setFillColor(bg)
    context.fill(CGRect(x: 0, y: 0, width: 512, height: 512))

    let auraGradient = CGGradient(
        colorsSpace: colorSpace,
        colors: [
            c(0.33, 0.96, 1.00, 0.22),
            c(0.49, 0.38, 1.00, 0.13),
            c(0.00, 0.00, 0.00, 0.00),
        ] as CFArray,
        locations: [0.0, 0.58, 1.0]
    )!
    context.drawRadialGradient(
        auraGradient,
        startCenter: CGPoint(x: 256, y: 256),
        startRadius: 12,
        endCenter: CGPoint(x: 256, y: 256),
        endRadius: 206,
        options: [.drawsAfterEndLocation]
    )

    let outerRect = CGRect(x: 105, y: 105, width: 302, height: 302)
    let outerPath = CGPath(roundedRect: outerRect, cornerWidth: 86, cornerHeight: 86, transform: nil)
    context.addPath(outerPath)
    context.setStrokeColor(c(0.35, 0.96, 1.00, 1.0))
    context.setLineWidth(18)
    context.strokePath()

    let markPath = CGMutablePath()
    markPath.move(to: CGPoint(x: 174, y: 330))
    markPath.addLine(to: CGPoint(x: 174, y: 184))
    markPath.addCurve(to: CGPoint(x: 196, y: 162), control1: CGPoint(x: 174, y: 172), control2: CGPoint(x: 184, y: 162))
    markPath.addLine(to: CGPoint(x: 229, y: 162))
    markPath.addCurve(to: CGPoint(x: 253, y: 174), control1: CGPoint(x: 247, y: 166), control2: CGPoint(x: 259, y: 171))
    markPath.addLine(to: CGPoint(x: 323, y: 265))
    markPath.addLine(to: CGPoint(x: 323, y: 184))
    markPath.addCurve(to: CGPoint(x: 345, y: 162), control1: CGPoint(x: 323, y: 172), control2: CGPoint(x: 333, y: 162))
    markPath.addLine(to: CGPoint(x: 377, y: 162))
    markPath.addCurve(to: CGPoint(x: 399, y: 184), control1: CGPoint(x: 389, y: 162), control2: CGPoint(x: 399, y: 172))
    markPath.addLine(to: CGPoint(x: 399, y: 330))
    markPath.addCurve(to: CGPoint(x: 377, y: 352), control1: CGPoint(x: 399, y: 342), control2: CGPoint(x: 389, y: 352))
    markPath.addLine(to: CGPoint(x: 345, y: 352))
    markPath.addCurve(to: CGPoint(x: 321, y: 340), control1: CGPoint(x: 327, y: 348), control2: CGPoint(x: 315, y: 343))
    markPath.addLine(to: CGPoint(x: 251, y: 249))
    markPath.addLine(to: CGPoint(x: 251, y: 330))
    markPath.addCurve(to: CGPoint(x: 229, y: 352), control1: CGPoint(x: 251, y: 342), control2: CGPoint(x: 241, y: 352))
    markPath.addLine(to: CGPoint(x: 196, y: 352))
    markPath.addCurve(to: CGPoint(x: 174, y: 330), control1: CGPoint(x: 184, y: 352), control2: CGPoint(x: 174, y: 342))
    markPath.closeSubpath()

    let markGradient = CGGradient(
        colorsSpace: colorSpace,
        colors: [
            c(0.35, 0.96, 1.00, 1.0),
            c(0.11, 0.58, 1.00, 1.0),
            c(0.51, 0.38, 1.00, 1.0),
            c(0.96, 0.94, 1.00, 1.0)
        ] as CFArray,
        locations: [0.0, 0.36, 0.72, 1.0]
    )!

    context.saveGState()
    context.addPath(markPath)
    context.clip()
    context.drawLinearGradient(
        markGradient,
        start: CGPoint(x: 140, y: 140),
        end: CGPoint(x: 390, y: 390),
        options: []
    )
    context.restoreGState()

    context.addPath(markPath)
    context.setStrokeColor(c(0.35, 0.96, 1.00, 0.95))
    context.setLineWidth(2)
    context.setLineJoin(.round)
    context.setLineCap(.round)
    context.strokePath()

    func line(_ start: CGPoint, _ end: CGPoint, color: CGColor) {
        context.beginPath()
        context.move(to: start)
        context.addLine(to: end)
        context.setStrokeColor(color)
        context.setLineWidth(12)
        context.setLineCap(.round)
        context.strokePath()
    }

    line(CGPoint(x: 139, y: 219), CGPoint(x: 80, y: 219), color: c(0.35, 0.96, 1.00, 1.0))
    line(CGPoint(x: 139, y: 268), CGPoint(x: 70, y: 268), color: c(0.11, 0.58, 1.00, 1.0))
    line(CGPoint(x: 139, y: 317), CGPoint(x: 85, y: 317), color: c(0.51, 0.38, 1.00, 1.0))
    line(CGPoint(x: 373, y: 219), CGPoint(x: 432, y: 219), color: c(0.35, 0.96, 1.00, 1.0))
    line(CGPoint(x: 373, y: 268), CGPoint(x: 442, y: 268), color: c(0.11, 0.58, 1.00, 1.0))
    line(CGPoint(x: 373, y: 317), CGPoint(x: 427, y: 317), color: c(0.51, 0.38, 1.00, 1.0))

    guard let image = context.makeImage() else {
        fail("klarte ikkje lage bitmap")
    }
    return image
}

func pngData(for image: CGImage) -> Data {
    let data = NSMutableData()
    guard let destination = CGImageDestinationCreateWithData(data, UTType.png.identifier as CFString, 1, nil) else {
        fail("klarte ikkje lage PNG-data")
    }
    CGImageDestinationAddImage(destination, image, nil)
    guard CGImageDestinationFinalize(destination) else {
        fail("klarte ikkje serialisere PNG-data")
    }
    return data as Data
}

let ext = output.pathExtension.lowercased()
if ext == "png" {
    let image = renderImage(size: size)
    guard let destination = CGImageDestinationCreateWithURL(output as CFURL, UTType.png.identifier as CFString, 1, nil) else {
        fail("klarte ikkje opne \(output.path) for skriving")
    }
    CGImageDestinationAddImage(destination, image, nil)
    guard CGImageDestinationFinalize(destination) else {
        fail("klarte ikkje skrive PNG")
    }
} else if ext == "ico" {
    let iconSizes = [16, 32, 48, 64, 128, 256]
    var pngs: [(size: Int, data: Data)] = []
    for iconSize in iconSizes {
        let data = pngData(for: renderImage(size: iconSize))
        pngs.append((iconSize, data))
    }

    var bytes = Data()
    func appendUInt16(_ value: UInt16) {
        var little = value.littleEndian
        withUnsafeBytes(of: &little) { bytes.append(contentsOf: $0) }
    }
    func appendUInt32(_ value: UInt32) {
        var little = value.littleEndian
        withUnsafeBytes(of: &little) { bytes.append(contentsOf: $0) }
    }

    appendUInt16(0)
    appendUInt16(1)
    appendUInt16(UInt16(pngs.count))

    let directorySize = 6 + (pngs.count * 16)
    var offset = UInt32(directorySize)
    for item in pngs {
        let s = item.size >= 256 ? 0 : UInt8(item.size)
        bytes.append(s)
        bytes.append(s)
        bytes.append(0)
        bytes.append(0)
        appendUInt16(1)
        appendUInt16(32)
        appendUInt32(UInt32(item.data.count))
        appendUInt32(offset)
        offset += UInt32(item.data.count)
    }

    for item in pngs {
        bytes.append(item.data)
    }

    try bytes.write(to: output)
} else {
    fail("ukjent utdataformat: \(ext)")
}
