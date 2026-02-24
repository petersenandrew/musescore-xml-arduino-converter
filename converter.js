"use strict";

const fs = require("fs");

const args = process.argv.slice(2);
const inputPath = args[0] || "bob.musicxml";
let outputPath = "song_data.h";
let inlineTarget = null;
let articulation = 0; // 0 = no gap, 1-10 = ticks of silence after each note
let appendMode = false;

// Parse flags
for (let i = 1; i < args.length; i++) {
	if (args[i] === "--inline") {
		inlineTarget = args[i + 1] || "arduino.ino";
		outputPath = null;
		i++;
	} else if (args[i].endsWith(".ino")) {
		inlineTarget = args[i];
		outputPath = null;
	} else if (args[i] === "--articulation" || args[i] === "-a") {
		articulation = Math.max(0, Math.min(10, Number(args[i + 1]) || 1));
		i++;
	} else if (args[i] === "--append") {
		appendMode = true;
		if (!inlineTarget) {
			inlineTarget = "arduino.ino";
			outputPath = null;
		}
	} else if (!args[i].startsWith("-") && !outputPath && !inlineTarget) {
		outputPath = args[i];
	}
}

function parseAttrs(attrText) {
	const attrs = {};
	const re = /([a-zA-Z_:][\w:.-]*)\s*=\s*"([^"]*)"/g;
	let match;
	while ((match = re.exec(attrText))) {
		attrs[match[1]] = match[2];
	}
	return attrs;
}

function midiToFreq(midi) {
	return Math.round(440 * Math.pow(2, (midi - 69) / 12));
}

function noteToMidi(step, alter, octave) {
	const stepMap = { C: 0, D: 2, E: 4, F: 5, G: 7, A: 9, B: 11 };
	const base = stepMap[step];
	if (base === undefined) return null;
	return (octave + 1) * 12 + base + (alter || 0);
}

function parseMusicXML(xml) {
	const tokens = xml.match(/<[^>]+>|[^<]+/g) || [];
	const parts = new Map();

	let currentPartId = null;
	let currentNote = null;
	let currentBackup = null;
	let textTarget = null;
	let tempo = null;
	let divisions = null;
	let timeBeats = null;
	let timeBeatType = null;

	function ensurePart(id) {
		if (!parts.has(id)) {
			parts.set(id, {
				events: new Map(),
				time: 0,
				lastStart: 0,
				divisions: null,
			});
		}
		return parts.get(id);
	}

	function addEvent(part, start, duration, midi) {
		const existing = part.events.get(start);
		if (!existing) {
			part.events.set(start, { duration, midi });
			return;
		}

		if (midi > existing.midi) {
			existing.midi = midi;
		}
		if (duration > existing.duration) {
			existing.duration = duration;
		}
	}

	function applyNote(note) {
		if (!currentPartId) return;
		if (!note) return;
		if (note.isGrace) return;
		const part = ensurePart(currentPartId);
		const duration = Number(note.duration || 0);
		if (duration <= 0) return;

		const start = note.chord ? part.lastStart : part.time;
		if (!note.chord) {
			part.lastStart = start;
		}

		if (note.isRest || note.step == null || note.octave == null) {
			if (!note.chord) {
				part.time += duration;
			}
			return;
		}

		const midi = noteToMidi(note.step, note.alter, note.octave);
		if (midi == null) return;

		addEvent(part, start, duration, midi);

		if (!note.chord) {
			part.time += duration;
		}
	}

	for (const token of tokens) {
		if (token.startsWith("<?") || token.startsWith("<!")) {
			continue;
		}

		if (token.startsWith("<")) {
			const isClose = token.startsWith("</");
			const isSelfClosing = token.endsWith("/>");
			const tagMatch = token.match(/^<\s*\/?([^\s/>]+)([^>]*)\/?\s*>$/);
			if (!tagMatch) continue;

			const tag = tagMatch[1];
			const attrText = tagMatch[2] || "";
			const attrs = parseAttrs(attrText);

			if (isClose) {
				if (tag === "note") {
					applyNote(currentNote);
					currentNote = null;
				} else if (tag === "backup") {
					if (currentBackup && currentPartId) {
						const part = ensurePart(currentPartId);
						part.time = Math.max(0, part.time - (currentBackup.duration || 0));
					}
					currentBackup = null;
				} else if (tag === "part") {
					currentPartId = null;
				}

				textTarget = null;
				continue;
			}

			if (tag === "part") {
				currentPartId = attrs.id || null;
				if (currentPartId) ensurePart(currentPartId);
			} else if (tag === "note") {
				currentNote = {
					isRest: false,
					chord: false,
					duration: null,
					step: null,
					alter: 0,
					octave: null,
					isGrace: false,
				};
			} else if (tag === "rest") {
				if (currentNote) currentNote.isRest = true;
			} else if (tag === "chord") {
				if (currentNote) currentNote.chord = true;
			} else if (tag === "grace") {
				if (currentNote) currentNote.isGrace = true;
			} else if (tag === "backup") {
				currentBackup = { duration: 0 };
			} else if (tag === "divisions") {
				textTarget = { type: "divisions" };
			} else if (tag === "beats") {
				textTarget = { type: "time-beats" };
			} else if (tag === "beat-type") {
				textTarget = { type: "time-beat-type" };
			} else if (tag === "sound") {
				if (attrs.tempo && tempo == null) tempo = Number(attrs.tempo);
			} else if (tag === "duration") {
				if (currentNote) {
					textTarget = { type: "note-duration" };
				} else if (currentBackup) {
					textTarget = { type: "backup-duration" };
				}
			} else if (tag === "step") {
				if (currentNote) textTarget = { type: "step" };
			} else if (tag === "alter") {
				if (currentNote) textTarget = { type: "alter" };
			} else if (tag === "octave") {
				if (currentNote) textTarget = { type: "octave" };
			}

			if (isSelfClosing) {
				if (tag === "rest" && currentNote) currentNote.isRest = true;
				if (tag === "chord" && currentNote) currentNote.chord = true;
				if (tag === "grace" && currentNote) currentNote.isGrace = true;
			}

			continue;
		}

		const text = token.trim();
		if (!text || !textTarget) continue;

		switch (textTarget.type) {
			case "divisions":
				if (currentPartId) {
					const part = ensurePart(currentPartId);
					if (part.divisions == null) {
						part.divisions = Number(text);
					}
				}
				if (divisions == null) divisions = Number(text);
				break;
			case "note-duration":
				if (currentNote) currentNote.duration = Number(text);
				break;
			case "time-beats":
				if (timeBeats == null) timeBeats = Number(text);
				break;
			case "time-beat-type":
				if (timeBeatType == null) timeBeatType = Number(text);
				break;
			case "backup-duration":
				if (currentBackup) currentBackup.duration = Number(text);
				break;
			case "step":
				if (currentNote) currentNote.step = text;
				break;
			case "alter":
				if (currentNote) currentNote.alter = Number(text);
				break;
			case "octave":
				if (currentNote) currentNote.octave = Number(text);
				break;
			default:
				break;
		}

		textTarget = null;
	}

	if (tempo == null || Number.isNaN(tempo)) tempo = 120;
	if (divisions == null || Number.isNaN(divisions) || divisions <= 0) divisions = 4;
	if (timeBeats == null || Number.isNaN(timeBeats) || timeBeats <= 0) timeBeats = 4;
	if (timeBeatType == null || Number.isNaN(timeBeatType) || timeBeatType <= 0) timeBeatType = 4;

	return { parts, tempo, divisions, timeBeats, timeBeatType };
}

function formatArray(items, columns, formatter) {
	const lines = [];
	let line = "  ";
	let count = 0;
	for (const item of items) {
		const text = formatter(item);
		if (count > 0) line += " ";
		line += text + ",";
		count++;
		if (count >= columns) {
			lines.push(line);
			line = "  ";
			count = 0;
		}
	}
	if (count > 0) lines.push(line);
	return lines.join("\n");
}

function applyArticulation(events, articulation) {
	if (articulation <= 0) return events;

	const result = [];
	for (let i = 0; i < events.length; i++) {
		const current = { ...events[i] };
		const next = i + 1 < events.length ? events[i + 1] : null;

		// If next note starts immediately after this one ends AND same pitch, shorten current note
		if (next && current.n === next.n && current.t + current.d === next.t) {
			current.d = Math.max(1, current.d - articulation);
		}

		result.push(current);
	}
	return result;
}

function buildOutput(parsed) {
	const parts = Array.from(parsed.parts.entries());
	const allEvents = [];

	for (const [, part] of parts) {
		const events = Array.from(part.events.entries())
			.map(([t, e]) => ({ t: Number(t), d: e.duration, midi: e.midi }))
			.sort((a, b) => a.t - b.t);
		part.sorted = events;
		allEvents.push(...events);
	}

	const uniqueMidi = Array.from(new Set(allEvents.map((e) => e.midi))).sort((a, b) => a - b);
	const midiToIndex = new Map();
	uniqueMidi.forEach((midi, idx) => midiToIndex.set(midi, idx));
	const freqs = uniqueMidi.map(midiToFreq);

	let totalTicks = 0;
	for (const [, part] of parts) {
		for (const ev of part.sorted) {
			totalTicks = Math.max(totalTicks, ev.t + ev.d);
		}
	}

	const headerLines = [];
	headerLines.push("#ifndef SONG_DATA_H");
	headerLines.push("#define SONG_DATA_H");
	headerLines.push("#include <Arduino.h>");
	headerLines.push("#include <avr/pgmspace.h>");
	headerLines.push("");
	headerLines.push("typedef struct { uint16_t t; uint16_t d; uint8_t n; } Event;");
	headerLines.push("");
	headerLines.push(`const uint16_t NOTE_FREQS[] PROGMEM = {`);
	headerLines.push(formatArray(freqs, 10, (v) => String(v)));
	headerLines.push("};");
	headerLines.push("");

	const partNames = [];
	parts.forEach(([id], idx) => {
		partNames.push({ id, name: `PART${idx + 1}` });
	});

	partNames.forEach((partName, index) => {
		const part = parts[index][1];
		let events = part.sorted.map((ev) => ({
			t: ev.t,
			d: ev.d,
			n: midiToIndex.get(ev.midi),
		}));
		events = applyArticulation(events, articulation);
		headerLines.push(`const Event ${partName.name}[] PROGMEM = {`);
		headerLines.push(
			formatArray(events, 3, (ev) => `{ ${ev.t}, ${ev.d}, ${ev.n} }`)
		);
		headerLines.push("};");
		headerLines.push(`const uint16_t ${partName.name}_LEN = sizeof(${partName.name}) / sizeof(${partName.name}[0]);`);
		headerLines.push("");
	});
	// Ensure we have at least 2 parts by creating empty ones if needed
	for (let i = partNames.length; i < 2; i++) {
		headerLines.push(`const Event PART${i + 1}[] PROGMEM = {};`);
		headerLines.push(`const uint16_t PART${i + 1}_LEN = 0;`);
		headerLines.push("");
	}
	headerLines.push(`const uint16_t TICKS_PER_QUARTER = ${parsed.divisions};`);
	headerLines.push(`const uint16_t TEMPO_BPM = ${parsed.tempo};`);
	headerLines.push(`const uint8_t TIME_BEATS = ${parsed.timeBeats};`);
	headerLines.push(`const uint8_t TIME_BEAT_TYPE = ${parsed.timeBeatType};`);
	headerLines.push(`const uint32_t TOTAL_TICKS = ${totalTicks};`);
	headerLines.push("");
	headerLines.push("#endif");

	return headerLines.join("\n");
}

function buildInlineBlock(parsed) {
	const parts = Array.from(parsed.parts.entries());
	const allEvents = [];

	for (const [, part] of parts) {
		const events = Array.from(part.events.entries())
			.map(([t, e]) => ({ t: Number(t), d: e.duration, midi: e.midi }))
			.sort((a, b) => a.t - b.t);
		part.sorted = events;
		allEvents.push(...events);
	}

	const uniqueMidi = Array.from(new Set(allEvents.map((e) => e.midi))).sort((a, b) => a - b);
	const midiToIndex = new Map();
	uniqueMidi.forEach((midi, idx) => midiToIndex.set(midi, idx));
	const freqs = uniqueMidi.map(midiToFreq);

	let totalTicks = 0;
	for (const [, part] of parts) {
		for (const ev of part.sorted) {
			totalTicks = Math.max(totalTicks, ev.t + ev.d);
		}
	}

	const lines = [];
	lines.push("// SONG_DATA_BEGIN");
	lines.push("typedef struct { uint16_t t; uint16_t d; uint8_t n; } Event;");
	lines.push("");
	lines.push("typedef struct {");
	lines.push("  const Event *part1;");
	lines.push("  uint16_t part1_len;");
	lines.push("  const Event *part2;");
	lines.push("  uint16_t part2_len;");
	lines.push("  uint16_t ticksPerQuarter;");
	lines.push("  uint16_t tempoBPM;");
	lines.push("  uint8_t timeBeats;");
	lines.push("  uint8_t timeBeatType;");
	lines.push("  uint32_t totalTicks;");
	lines.push("} Song;");
	lines.push("");
	lines.push(`const uint16_t NOTE_FREQS[] PROGMEM = {`);
	lines.push(formatArray(freqs, 10, (v) => String(v)));
	lines.push("};");
	lines.push("");

	const partNames = [];
	parts.forEach(([id], idx) => {
		partNames.push({ id, name: `PART${idx + 1}` });
	});

	partNames.forEach((partName, index) => {
		const part = parts[index][1];
		let events = part.sorted.map((ev) => ({
			t: ev.t,
			d: ev.d,
			n: midiToIndex.get(ev.midi),
		}));
		events = applyArticulation(events, articulation);
		lines.push(`const Event ${partName.name}[] PROGMEM = {`);
		lines.push(
			formatArray(events, 3, (ev) => `{ ${ev.t}, ${ev.d}, ${ev.n} }`)
		);
		lines.push("};");
		lines.push(`const uint16_t ${partName.name}_LEN = sizeof(${partName.name}) / sizeof(${partName.name}[0]);`);
		lines.push("");
	});
	lines.push("");
	// Ensure we have at least 2 parts by creating empty ones if needed
	for (let i = partNames.length; i < 2; i++) {
		lines.push(`const Event PART${i + 1}[] PROGMEM = {};`);
		lines.push(`const uint16_t PART${i + 1}_LEN = 0;`);
		lines.push("");
	}
	lines.push("// ===== SONG ARRAY =====");
	lines.push("const Song SONGS[] = {");
	lines.push("  // Song 1");
	lines.push("  {");
	lines.push(`    PART1, PART1_LEN,`);
	lines.push(`    PART2, PART2_LEN,`);
	lines.push(`    ${parsed.divisions},    // ticksPerQuarter`);
	lines.push(`    ${parsed.tempo},  // tempoBPM`);
	lines.push(`    ${parsed.timeBeats},    // timeBeats`);
	lines.push(`    ${parsed.timeBeatType},    // timeBeatType`);
	lines.push(`    ${totalTicks}   // totalTicks`);
	lines.push("  },");
	lines.push("};");
	lines.push("const uint8_t NUM_SONGS = sizeof(SONGS) / sizeof(SONGS[0]);");
	lines.push("");
	lines.push("typedef struct {");
	lines.push("  byte uid[4];");
	lines.push("  int8_t songIndex;");
	lines.push("} RFIDMapping;");
	lines.push("");
	lines.push("const RFIDMapping RFID_MAPPINGS[] = {");
	lines.push("  // Example mappings - replace with your actual RFID UIDs");
	lines.push("  { { 0x63, 0x4A, 0xE9, 0xFB }, 0 },  // Card 1 -> Song 1");
	lines.push("  { { 0x1B, 0x79, 0xF3, 0x00 }, 1 },  // Card 2 -> Song 2");
	lines.push("};");
	lines.push("const uint8_t NUM_RFID_MAPPINGS = sizeof(RFID_MAPPINGS) / sizeof(RFID_MAPPINGS[0]);");
	lines.push("");
	lines.push("const uint8_t SONG_TO_PLAY = 255;");
	lines.push("// SONG_DATA_END");

	return lines.join("\n");
}

function writeInlineData(inoPath, block) {
	const begin = "// SONG_DATA_BEGIN";
	const end = "// SONG_DATA_END";
	const content = fs.readFileSync(inoPath, "utf8");
	const start = content.indexOf(begin);
	const stop = content.indexOf(end);
	if (start === -1 || stop === -1 || stop < start) {
		throw new Error(`Missing ${begin}/${end} markers in ${inoPath}`);
	}
	const before = content.slice(0, start);
	const after = content.slice(stop + end.length);
	const updated = `${before}${block}${after}`;
	fs.writeFileSync(inoPath, updated, "utf8");
}

function getNextPartNumber(content) {
	const partRegex = /const\s+Event\s+PART(\d+)\[\]/g;
	let maxPart = 0;
	let match;
	while ((match = partRegex.exec(content))) {
		const partNum = parseInt(match[1], 10);
		if (partNum > maxPart) maxPart = partNum;
	}
	return maxPart + 1;
}

function parseNoteFreqs(content) {
	const match = content.match(/const\s+uint16_t\s+NOTE_FREQS\[\]\s+PROGMEM\s*=\s*\{([^}]+)\}/);
	if (!match) return [];
	const freqsText = match[1];
	const freqs = freqsText.split(',').map(s => s.trim()).filter(s => s).map(Number);
	return freqs;
}

function mergeFrequencies(existingFreqs, newMidiNotes) {
	const freqSet = new Set(existingFreqs);
	const newFreqs = newMidiNotes.map(midiToFreq);
	
	for (const freq of newFreqs) {
		freqSet.add(freq);
	}
	
	return Array.from(freqSet).sort((a, b) => a - b);
}

function appendSongToFile(inoPath, parsed) {
	const content = fs.readFileSync(inoPath, "utf8");
	const begin = "// SONG_DATA_BEGIN";
	const end = "// SONG_DATA_END";
	
	const startIdx = content.indexOf(begin);
	const endIdx = content.indexOf(end);
	if (startIdx === -1 || endIdx === -1) {
		throw new Error(`Missing ${begin}/${end} markers in ${inoPath}`);
	}

	const nextPartNum = getNextPartNumber(content);
	
	// Parse existing NOTE_FREQS
	const existingFreqs = parseNoteFreqs(content);

	// Build new song parts
	const parts = Array.from(parsed.parts.entries());
	const allEvents = [];

	for (const [, part] of parts) {
		const events = Array.from(part.events.entries())
			.map(([t, e]) => ({ t: Number(t), d: e.duration, midi: e.midi }))
			.sort((a, b) => a.t - b.t);
		part.sorted = events;
		allEvents.push(...events);
	}

	const uniqueMidi = Array.from(new Set(allEvents.map((e) => e.midi))).sort((a, b) => a - b);
	
	// Merge frequencies and regenerate NOTE_FREQS if needed
	const allFreqs = mergeFrequencies(existingFreqs, uniqueMidi);
	const freqToIndex = new Map();
	allFreqs.forEach((freq, idx) => freqToIndex.set(freq, idx));
	
	const midiToIndex = new Map();
	uniqueMidi.forEach((midi) => {
		const freq = midiToFreq(midi);
		midiToIndex.set(midi, freqToIndex.get(freq));
	});

	let totalTicks = 0;
	for (const [, part] of parts) {
		for (const ev of part.sorted) {
			totalTicks = Math.max(totalTicks, ev.t + ev.d);
		}
	}

	// Generate new PARTs
	const newPartsLines = [];
	newPartsLines.push(`\n// ===== SONG ${Math.floor(nextPartNum / 2) + 1} DATA =====\n`);
	
	const newPartNames = [];
	parts.forEach((_, idx) => {
		newPartNames.push(`PART${nextPartNum + idx}`);
	});

	newPartNames.forEach((partName, index) => {
		const part = parts[index][1];
		let events = part.sorted.map((ev) => ({
			t: ev.t,
			d: ev.d,
			n: midiToIndex.get(ev.midi),
		}));
		events = applyArticulation(events, articulation);
		newPartsLines.push(`const Event ${partName}[] PROGMEM = {`);
		newPartsLines.push(
			formatArray(events, 3, (ev) => `{ ${ev.t}, ${ev.d}, ${ev.n} }`)
		);
		newPartsLines.push("};");
		newPartsLines.push(`const uint16_t ${partName}_LEN = sizeof(${partName}) / sizeof(${partName}[0]);`);
		newPartsLines.push("");
	});
	// Ensure we have at least 2 parts by creating empty ones if needed
	for (let i = newPartNames.length; i < 2; i++) {
		const emptyPartName = `PART${nextPartNum + i}`;
		newPartsLines.push(`const Event ${emptyPartName}[] PROGMEM = {};`);
		newPartsLines.push(`const uint16_t ${emptyPartName}_LEN = 0;`);
		newPartsLines.push("");
		newPartNames.push(emptyPartName);
	}
	let dataSection = content.substring(startIdx + begin.length, endIdx);
	
	// Update NOTE_FREQS if we added new frequencies
	if (allFreqs.length > existingFreqs.length) {
		const newFreqsText = formatArray(allFreqs, 10, (v) => String(v));
		const freqsRegex = /(const\s+uint16_t\s+NOTE_FREQS\[\]\s+PROGMEM\s*=\s*\{)[\s\S]*?(\};)/;
		dataSection = dataSection.replace(freqsRegex, `$1\n${newFreqsText}\n$2`);
	}
	
	// Find where to insert new parts (before SONG ARRAY)
	const songArrayMarker = "// ===== SONG ARRAY =====";
	const songArrayIdx = dataSection.indexOf(songArrayMarker);
	
	if (songArrayIdx === -1) {
		throw new Error("Could not find SONG ARRAY marker");
	}

	// Insert new parts before SONG ARRAY
	const beforeSongArray = dataSection.substring(0, songArrayIdx);
	const afterSongArray = dataSection.substring(songArrayIdx);

	// Find the SONGS array and add new entry
	const songsMatch = afterSongArray.match(/(\/\/ ===== SONG ARRAY =====\s*const\s+Song\s+SONGS\[\]\s*=\s*\{[\s\S]*?)(};)/);
	if (!songsMatch) {
		throw new Error("Could not find SONGS array");
	}

	const songsArrayContent = songsMatch[1];
	const closingBrace = songsMatch[2];
	
	// Build new song entry (ensure at least 2 parts)
	const part1Name = newPartNames[0] || `PART${nextPartNum}`;
	const part2Name = newPartNames[1] || `PART${nextPartNum + 1}`;
	const newSongEntry = `  // Song ${Math.floor(nextPartNum / 2) + 1}\n  {\n    ${part1Name}, ${part1Name}_LEN,\n    ${part2Name}, ${part2Name}_LEN,\n    ${parsed.divisions},    // ticksPerQuarter\n    ${parsed.tempo},  // tempoBPM\n    ${parsed.timeBeats},    // timeBeats\n    ${parsed.timeBeatType},    // timeBeatType\n    ${totalTicks}   // totalTicks\n  },\n`;

	// Reconstruct the SONGS array
	const newSongsArray = songsArrayContent + newSongEntry + closingBrace;
	const restOfDataSection = afterSongArray.substring(songsMatch[0].length);

	// Reconstruct full data section
	const newDataSection = beforeSongArray + newPartsLines.join("\n") + newSongsArray + restOfDataSection;

	// Reconstruct entire file
	const before = content.substring(0, startIdx + begin.length);
	const after = content.substring(endIdx);
	const updated = before + newDataSection + after;

	fs.writeFileSync(inoPath, updated, "utf8");

	return Math.floor(nextPartNum / 2) + 1;
}

const xml = fs.readFileSync(inputPath, "utf8");
const parsed = parseMusicXML(xml);
const partIds = Array.from(parsed.parts.keys()).join(", ");

if (inlineTarget) {
	if (appendMode) {
		const songNumber = appendSongToFile(inlineTarget, parsed);
		const artStr = articulation > 0 ? `, articulation ${articulation} ticks` : "";
		console.log(
			`Appended Song ${songNumber} to ${inlineTarget} (parts: ${partIds || "none"}, tempo ${parsed.tempo}, divisions ${parsed.divisions}${artStr}).`
		);
	} else {
		const block = buildInlineBlock(parsed);
		writeInlineData(inlineTarget, block);
		const artStr = articulation > 0 ? `, articulation ${articulation} ticks` : "";
		console.log(
			`Inlined song data into ${inlineTarget} (parts: ${partIds || "none"}, tempo ${parsed.tempo}, divisions ${parsed.divisions}${artStr}).`
		);
	}
} else {
	const output = buildOutput(parsed);
	fs.writeFileSync(outputPath, output, "utf8");
	const artStr = articulation > 0 ? `, articulation ${articulation} ticks` : "";
	console.log(
		`Wrote ${outputPath} (parts: ${partIds || "none"}, tempo ${parsed.tempo}, divisions ${parsed.divisions}${artStr}).`
	);
}
