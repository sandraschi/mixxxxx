"""Add VideoWidget parser implementation to legacyskinparser.cpp."""
import sys

cpp_path = "D:/Dev/repos/mixxxxx/src/skin/legacy/legacyskinparser.cpp"
with open(cpp_path, 'r', encoding='utf-8') as f:
    content = f.read()

# The exact text between parseSpinny end and parseVuMeter start
old = (
    "    pSpinny->Init();\n"
    "    return pSpinny;\n"
    "}\n"
    "\n"
    "QWidget* LegacySkinParser::parseVuMeter(const QDomElement& node) {"
)

new = (
    "    pSpinny->Init();\n"
    "    return pSpinny;\n"
    "}\n"
    "\n"
    "QWidget* LegacySkinParser::parseVideoWidget(const QDomElement& node) {\n"
    "#ifdef MIXXX_USE_QML\n"
    "    if (CmdlineArgs::Instance().isQml()) {\n"
    "        return nullptr;\n"
    "    }\n"
    "#endif\n"
    "    if (CmdlineArgs::Instance().getSafeMode()) {\n"
    "        WLabel* dummy = new WLabel(m_pParent);\n"
    '        dummy->setText(tr("Safe Mode Enabled"));\n'
    "        return dummy;\n"
    "    }\n"
    "\n"
    "    QString group = lookupNodeGroup(node);\n"
    "    VideoWidget* widget = new VideoWidget(group, m_pParent);\n"
    "    commonWidgetSetup(node, widget);\n"
    "\n"
    "    BaseTrackPlayer* pPlayer = m_pPlayerManager->getPlayer(group);\n"
    "    if (pPlayer) {\n"
    "        QObject::connect(pPlayer, &BaseTrackPlayer::newTrackLoaded,\n"
    "                widget, &VideoWidget::slotLoadTrack);\n"
    "    }\n"
    "\n"
    "    return widget;\n"
    "}\n"
    "\n"
    "QWidget* LegacySkinParser::parseVuMeter(const QDomElement& node) {"
)

if old in content:
    content = content.replace(old, new, 1)
    with open(cpp_path, 'w', encoding='utf-8') as f:
        f.write(content)
    print("OK: VideoWidget parser added")
else:
    print("ERROR: Pattern not found")
    # Debug: show context around the expected insertion point
    idx = content.find("pSpinny->Init()")
    if idx >= 0:
        print(f"Found pSpinny->Init() at offset {idx}")
        print("Context:")
        print(repr(content[idx:idx+200]))
    sys.exit(1)
