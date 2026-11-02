import AppKit
import CoreGraphics
import Foundation

struct RGBColor {
    let red: CGFloat
    let green: CGFloat
    let blue: CGFloat
    let alpha: CGFloat

    init(_ red: CGFloat, _ green: CGFloat, _ blue: CGFloat, _ alpha: CGFloat = 1) {
        self.red = red / 255
        self.green = green / 255
        self.blue = blue / 255
        self.alpha = alpha
    }

    var cgColor: CGColor {
        CGColor(red: red, green: green, blue: blue, alpha: alpha)
    }
}

let outputDirectory = URL(fileURLWithPath: FileManager.default.currentDirectoryPath)
    .appendingPathComponent("Resources/Assets.xcassets/AppIcon.appiconset")
let canvasSize = CGSize(width: 256, height: 256)

func rect(_ x: CGFloat, _ y: CGFloat, _ width: CGFloat, _ height: CGFloat) -> CGRect {
    CGRect(x: x, y: y, width: width, height: height)
}

func point(_ x: CGFloat, _ y: CGFloat) -> CGPoint {
    CGPoint(x: x, y: y)
}

func path(points: [CGPoint]) -> CGPath {
    let path = CGMutablePath()
    guard let first = points.first else { return path }
    path.move(to: first)
    for point in points.dropFirst() {
        path.addLine(to: point)
    }
    path.closeSubpath()
    return path
}

func drawIcon(sideLength: CGFloat) -> NSBitmapImageRep {
    let bitmap = NSBitmapImageRep(
        bitmapDataPlanes: nil,
        pixelsWide: Int(sideLength),
        pixelsHigh: Int(sideLength),
        bitsPerSample: 8,
        samplesPerPixel: 4,
        hasAlpha: true,
        isPlanar: false,
        colorSpaceName: .deviceRGB,
        bytesPerRow: 0,
        bitsPerPixel: 0
    )!

    let graphicsContext = NSGraphicsContext(bitmapImageRep: bitmap)!
    NSGraphicsContext.saveGraphicsState()
    NSGraphicsContext.current = graphicsContext

    let context = graphicsContext.cgContext
    context.clear(CGRect(origin: .zero, size: CGSize(width: sideLength, height: sideLength)))
    context.scaleBy(x: sideLength / canvasSize.width, y: sideLength / canvasSize.height)
    context.setAllowsAntialiasing(true)
    context.setShouldAntialias(true)

    let iconRect = rect(18, 18, 220, 220)
    let iconPath = CGPath(
        roundedRect: iconRect,
        cornerWidth: 55,
        cornerHeight: 55,
        transform: nil
    )

    context.saveGState()
    context.addPath(iconPath)
    context.clip()

    let backgroundGradient = CGGradient(
        colorsSpace: CGColorSpaceCreateDeviceRGB(),
        colors: [
            RGBColor(16, 34, 63).cgColor,
            RGBColor(21, 128, 202).cgColor,
            RGBColor(35, 197, 142).cgColor
        ] as CFArray,
        locations: [0, 0.58, 1]
    )
    context.drawLinearGradient(
        backgroundGradient!,
        start: point(16, 15),
        end: point(256, 260),
        options: []
    )

    context.setFillColor(RGBColor(255, 255, 255, 0.12).cgColor)
    context.fillEllipse(in: rect(164, 4, 116, 116))
    context.setFillColor(RGBColor(255, 255, 255, 0.08).cgColor)
    context.fillEllipse(in: rect(-16, 184, 124, 124))

    let backPhoto = CGPath(
        roundedRect: rect(42, 44, 174, 138),
        cornerWidth: 22,
        cornerHeight: 22,
        transform: nil
    )
    context.setShadow(
        offset: CGSize(width: 0, height: -8),
        blur: 16,
        color: RGBColor(5, 17, 38, 0.24).cgColor
    )
    context.setFillColor(RGBColor(236, 248, 255, 0.32).cgColor)
    context.addPath(backPhoto)
    context.fillPath()
    context.setShadow(offset: .zero, blur: 0)

    let photoPath = CGPath(
        roundedRect: rect(32, 62, 184, 140),
        cornerWidth: 22,
        cornerHeight: 22,
        transform: nil
    )
    context.saveGState()
    context.addPath(photoPath)
    context.clip()

    let photoGradient = CGGradient(
        colorsSpace: CGColorSpaceCreateDeviceRGB(),
        colors: [
            RGBColor(236, 250, 255).cgColor,
            RGBColor(151, 219, 238).cgColor,
            RGBColor(45, 99, 178).cgColor
        ] as CFArray,
        locations: [0, 0.48, 1]
    )
    context.drawLinearGradient(
        photoGradient!,
        start: point(34, 200),
        end: point(216, 64),
        options: []
    )

    context.setFillColor(RGBColor(255, 222, 108).cgColor)
    context.fillEllipse(in: rect(158, 78, 38, 38))

    context.setFillColor(RGBColor(24, 72, 114, 0.92).cgColor)
    context.addPath(path(points: [
        point(25, 204),
        point(74, 138),
        point(105, 169),
        point(132, 129),
        point(218, 204)
    ]))
    context.fillPath()

    context.setFillColor(RGBColor(13, 45, 82, 0.96).cgColor)
    context.addPath(path(points: [
        point(74, 204),
        point(132, 126),
        point(171, 174),
        point(222, 122),
        point(230, 204)
    ]))
    context.fillPath()

    context.restoreGState()

    context.setStrokeColor(RGBColor(255, 255, 255, 0.72).cgColor)
    context.setLineWidth(4)
    context.addPath(photoPath)
    context.strokePath()

    context.setStrokeColor(RGBColor(255, 255, 255, 0.9).cgColor)
    context.setLineWidth(14)
    context.setLineCap(.round)
    context.setLineJoin(.round)
    let mark = CGMutablePath()
    mark.move(to: point(64, 92))
    mark.addLine(to: point(96, 162))
    mark.addLine(to: point(124, 108))
    mark.addLine(to: point(153, 162))
    mark.addLine(to: point(188, 92))
    context.addPath(mark)
    context.strokePath()

    context.setStrokeColor(RGBColor(12, 35, 68, 0.22).cgColor)
    context.setLineWidth(3)
    context.addPath(mark)
    context.strokePath()

    context.restoreGState()

    context.setStrokeColor(RGBColor(255, 255, 255, 0.2).cgColor)
    context.setLineWidth(2)
    context.addPath(iconPath)
    context.strokePath()

    NSGraphicsContext.restoreGraphicsState()
    return bitmap
}

func writePNG(_ bitmap: NSBitmapImageRep, to url: URL) throws {
    guard let pngData = bitmap.representation(using: .png, properties: [:]) else {
        throw CocoaError(.fileWriteUnknown)
    }

    try pngData.write(to: url)
}

let outputs: [(String, CGFloat)] = [
    ("icon_16x16.png", 16),
    ("icon_16x16@2x.png", 32),
    ("icon_32x32.png", 32),
    ("icon_32x32@2x.png", 64),
    ("icon_128x128.png", 128),
    ("icon_128x128@2x.png", 256),
    ("icon_256x256.png", 256),
    ("icon_256x256@2x.png", 512),
    ("icon_512x512.png", 512),
    ("icon_512x512@2x.png", 1024)
]

try FileManager.default.createDirectory(at: outputDirectory, withIntermediateDirectories: true)

for (filename, sideLength) in outputs {
    let bitmap = drawIcon(sideLength: sideLength)
    try writePNG(bitmap, to: outputDirectory.appendingPathComponent(filename))
}

print("Generated app icons in \(outputDirectory.path)")
