import QtQuick
import QtQuick.Controls.FluentWinUI3

ComboBox {
    id: control

    // Calculate width based on the longest text in the model
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                           textMetrics.width + leftPadding + rightPadding + indicator.width + spacing)

    TextMetrics {
        id: textMetrics
        font: control.font
        text: {
            if (!control.model || control.model.length === 0) {
                return ""
            }

            let longestText = ""
            for (let i = 0; i < control.model.length; i++) {
                let itemText = control.model[i]
                if (typeof itemText === "object") {
                    // Handle model items that are objects (e.g., with 'text' property)
                    itemText = itemText.text || itemText.toString()
                }
                if (itemText.length > longestText.length) {
                    longestText = itemText
                }
            }
            return longestText
        }
    }

    // Add some extra padding to ensure comfortable spacing
    leftPadding: 12
    rightPadding: 12
    spacing: 8

    // Make sure the popup width matches the control width
    popup.width: control.width
}
