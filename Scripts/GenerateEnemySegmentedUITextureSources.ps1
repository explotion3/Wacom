param(
    [string]$ProjectRoot = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$outputDirectory = Join-Path $ProjectRoot 'Saved\WacomGenerated\EnemyUI'
[System.IO.Directory]::CreateDirectory($outputDirectory) | Out-Null

function New-TransparentBitmap {
    param([int]$Width, [int]$Height)

    $bitmap = [System.Drawing.Bitmap]::new(
        $Width,
        $Height,
        [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $bitmap.SetResolution(96.0, 96.0)
    return $bitmap
}

function Set-PixelBlock {
    param(
        [System.Drawing.Bitmap]$Bitmap,
        [System.Drawing.Color]$Color,
        [int]$X,
        [int]$Y,
        [int]$Width = 1,
        [int]$Height = 1
    )

    for ($offsetY = 0; $offsetY -lt $Height; ++$offsetY) {
        for ($offsetX = 0; $offsetX -lt $Width; ++$offsetX) {
            $pixelX = $X + $offsetX
            $pixelY = $Y + $offsetY
            if ($pixelX -ge 0 -and $pixelX -lt $Bitmap.Width -and
                $pixelY -ge 0 -and $pixelY -lt $Bitmap.Height) {
                $Bitmap.SetPixel($pixelX, $pixelY, $Color)
            }
        }
    }
}

$cyan = [System.Drawing.Color]::FromArgb(255, 82, 209, 255)
$bright = [System.Drawing.Color]::FromArgb(255, 214, 249, 255)
$deep = [System.Drawing.Color]::FromArgb(255, 18, 71, 110)

$badge = New-TransparentBitmap -Width 24 -Height 24
try {
    # 对称像素盾牌轮廓；完全由 Wacom 项目生成，不来自外部参考图。
    foreach ($row in @(
        @(4, 4, 16), @(3, 5, 18), @(3, 6, 2), @(19, 6, 2),
        @(2, 7, 2), @(20, 7, 2), @(2, 8, 2), @(20, 8, 2),
        @(2, 9, 2), @(20, 9, 2), @(3, 10, 2), @(19, 10, 2),
        @(3, 11, 2), @(19, 11, 2), @(4, 12, 2), @(18, 12, 2),
        @(5, 13, 2), @(17, 13, 2), @(6, 14, 2), @(16, 14, 2),
        @(7, 15, 2), @(15, 15, 2), @(8, 16, 2), @(14, 16, 2),
        @(9, 17, 2), @(13, 17, 2), @(10, 18, 4))) {
        Set-PixelBlock -Bitmap $badge -Color $cyan -X $row[0] -Y $row[1] -Width $row[2]
    }
    Set-PixelBlock -Bitmap $badge -Color $bright -X 6 -Y 6 -Width 12 -Height 2
    Set-PixelBlock -Bitmap $badge -Color $deep -X 7 -Y 9 -Width 10 -Height 4
    Set-PixelBlock -Bitmap $badge -Color $bright -X 10 -Y 10 -Width 4 -Height 6
    $badgePath = Join-Path $outputDirectory 'T_UI_EnemyShieldBadge.png'
    $badge.Save($badgePath, [System.Drawing.Imaging.ImageFormat]::Png)
}
finally {
    $badge.Dispose()
}

$frame = New-TransparentBitmap -Width 48 -Height 48
try {
    # 8 px 九宫格边缘；中心保持透明，Overlay 不会遮住 HP 填充。
    for ($index = 0; $index -lt 48; ++$index) {
        Set-PixelBlock -Bitmap $frame -Color $deep -X $index -Y 1 -Width 1 -Height 2
        Set-PixelBlock -Bitmap $frame -Color $deep -X $index -Y 45 -Width 1 -Height 2
        Set-PixelBlock -Bitmap $frame -Color $deep -X 1 -Y $index -Width 2 -Height 1
        Set-PixelBlock -Bitmap $frame -Color $deep -X 45 -Y $index -Width 2 -Height 1
        Set-PixelBlock -Bitmap $frame -Color $cyan -X $index -Y 3 -Width 1 -Height 2
        Set-PixelBlock -Bitmap $frame -Color $cyan -X $index -Y 43 -Width 1 -Height 2
        Set-PixelBlock -Bitmap $frame -Color $cyan -X 3 -Y $index -Width 2 -Height 1
        Set-PixelBlock -Bitmap $frame -Color $cyan -X 43 -Y $index -Width 2 -Height 1
    }
    Set-PixelBlock -Bitmap $frame -Color $bright -X 6 -Y 3 -Width 10 -Height 1
    Set-PixelBlock -Bitmap $frame -Color $bright -X 3 -Y 6 -Width 1 -Height 10
    Set-PixelBlock -Bitmap $frame -Color $bright -X 32 -Y 44 -Width 10 -Height 1
    Set-PixelBlock -Bitmap $frame -Color $bright -X 44 -Y 32 -Width 1 -Height 10
    $framePath = Join-Path $outputDirectory 'T_UI_EnemyShieldFrame_9Slice.png'
    $frame.Save($framePath, [System.Drawing.Imaging.ImageFormat]::Png)
}
finally {
    $frame.Dispose()
}

Write-Output $outputDirectory
