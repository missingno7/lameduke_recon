// Load the pinned LE image and export analysis drafts.
// @category LameDuke

import com.google.gson.*;
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.database.SpecExtension;
import ghidra.program.model.address.*;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.reloc.Relocation;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;

public class SeedLameDuke extends GhidraScript {
    private static final Gson JSON = new GsonBuilder().setPrettyPrinting().disableHtmlEscaping().create();
    private static final SourceType USER = SourceType.USER_DEFINED;
    private static final SourceType IMPORTED = SourceType.IMPORTED;
    private Path work;
    private JsonObject input;
    private final Map<String, DataType> structureTypes = new LinkedHashMap<>();
    private final Map<Address, String> globalNames = new TreeMap<>();
    private final NavigableMap<Address, Address> fixups = new TreeMap<>();
    private int codeStart;
    private int codeEnd;
    private int dataStart;
    private int dataEnd;

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        work = Path.of(args[1]);
        try (Reader reader = Files.newBufferedReader(work.resolve("input.json"))) {
            input = JsonParser.parseReader(reader).getAsJsonObject();
        }
        JsonArray code = array("code");
        JsonArray data = array("data");
        codeStart = code.get(0).getAsInt();
        codeEnd = codeStart + code.get(1).getAsInt();
        dataStart = data.get(0).getAsInt();
        dataEnd = dataStart + data.get(1).getAsInt();
        for (JsonElement row : array("fixups")) {
            JsonArray fixup = row.getAsJsonArray();
            fixups.put(addr(fixup.get(0).getAsInt()), addr(fixup.get(1).getAsInt()));
        }
        if ("prepare".equals(args[0])) {
            prepare();
        }
        else if ("export".equals(args[0])) {
            export();
        }
        else {
            throw new IllegalArgumentException("Expected prepare or export");
        }
    }

    private JsonArray array(String key) {
        return input.getAsJsonArray(key);
    }

    private JsonObject object(String key) {
        return input.getAsJsonObject(key);
    }

    private Address addr(long linear) {
        return currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(linear);
    }

    private Address entry(JsonObject seed) {
        return addr(seed.get("linear").getAsInt());
    }

    private String coordinate(Address address) {
        long offset = address.getOffset();
        if (offset >= codeStart && offset < codeEnd) {
            return String.format("1:%05x", offset - codeStart);
        }
        if (offset >= dataStart && offset < dataEnd) {
            return String.format("2:%05x", offset - dataStart);
        }
        return address.toString();
    }

    private void prepare() throws Exception {
        installSegments();
        Path script = getSourceFile().getFile(false).toPath();
        new SpecExtension(currentProgram).addReplaceCompilerSpecExtension(
            Files.readString(script.resolveSibling("watcall.xml")), monitor);
        // Make body recovery start from project-owned seeds and existing
        // LE pointers, with no external function-ID databases.
        setAnalysisOption(currentProgram, "Function ID", "false");
        setAnalysisOption(currentProgram, "Decompiler Parameter ID", "false");
        applyRelocations();
        installNames();
        installTypes();
        for (JsonElement row : array("code_data")) {
            JsonArray range = row.getAsJsonArray();
            currentProgram.getListing().createData(addr(range.get(0).getAsInt()),
                arrayType(ByteDataType.dataType, range.get(1).getAsInt()));
        }
        markJumpTables();
        seedFunctions();
        attachReferences();
    }

    private void installSegments() throws Exception {
        Memory memory = currentProgram.getMemory();
        Address bss = addr(input.get("bss").getAsInt());
        for (Address boundary : List.of(addr(codeStart), addr(codeEnd), addr(dataStart), bss)) {
            memory.split(memory.getBlock(boundary), boundary);
        }
        memory.removeBlock(memory.getBlock(addr(0)), monitor);
        memory.removeBlock(memory.getBlock(addr(codeEnd)), monitor);
        segment(memory.getBlock(addr(codeStart)), ".text", false, true);
        segment(memory.getBlock(addr(dataStart)), ".data", true, false);
        segment(memory.getBlock(bss), ".bss", true, false);
        currentProgram.getSymbolTable().addExternalEntryPoint(addr(input.get("entry").getAsInt()));
    }

    private void segment(MemoryBlock block, String name, boolean write, boolean execute)
            throws Exception {
        block.setName(name);
        block.setRead(true);
        block.setWrite(write);
        block.setExecute(execute);
    }

    private void applyRelocations() throws Exception {
        Memory memory = currentProgram.getMemory();
        for (JsonElement row : array("fixups")) {
            JsonArray fixup = row.getAsJsonArray();
            Address from = addr(fixup.get(0).getAsInt());
            Address to = addr(fixup.get(1).getAsInt());
            byte[] original = new byte[4];
            memory.getBytes(from, original);
            memory.setInt(from, (int) to.getOffset());
            currentProgram.getRelocationTable().add(from, Relocation.Status.APPLIED,
                fixup.get(2).getAsInt(), new long[] {to.getOffset()}, original, null);
        }
        println("Installed " + fixups.size() + " LE relocation sites");
    }

    private void installNames() throws Exception {
        SymbolTable table = currentProgram.getSymbolTable();
        Namespace global = currentProgram.getGlobalNamespace();
        int count = 0;
        for (var row : object("names").entrySet()) {
            Address address = addr(Long.parseLong(row.getKey()));
            // A canonical data binding at a code address can mark an extent
            // boundary. Preserve it as a label as well.
            boolean first = true;
            for (JsonElement alias : row.getValue().getAsJsonArray()) {
                JsonArray fields = alias.getAsJsonArray();
                String name = fields.get(0).getAsString();
                if (table.getSymbol(name, address, global) == null) {
                    Symbol symbol = table.createLabel(address, name, IMPORTED);
                    if (first) {
                        symbol.setPrimary();
                    }
                    count++;
                }
                first = false;
            }
        }
        println("Installed " + count + " distinct named addresses and aliases");
    }

    private DataType type(String name) {
        DataTypeManager manager = currentProgram.getDataTypeManager();
        if (name.endsWith(" *")) {
            return new PointerDataType(type(name.substring(0, name.length() - 2)), manager);
        }
        if (structureTypes.containsKey(name)) {
            return structureTypes.get(name);
        }
        return switch (name) {
            case "void" -> VoidDataType.dataType;
            case "char" -> CharDataType.dataType;
            case "unsigned char" -> UnsignedCharDataType.dataType;
            case "signed char" -> SignedCharDataType.dataType;
            case "short" -> ShortDataType.dataType;
            case "unsigned short", "short unsigned int" -> UnsignedShortDataType.dataType;
            case "int" -> IntegerDataType.dataType;
            case "unsigned int" -> UnsignedIntegerDataType.dataType;
            case "long" -> LongDataType.dataType;
            case "unsigned long" -> UnsignedLongDataType.dataType;
            default -> throw new IllegalArgumentException("Unsupported type " + name);
        };
    }

    private DataType arrayType(DataType element, int count) {
        element = element.clone(currentProgram.getDataTypeManager());
        return count == 1 ? element : new ArrayDataType(element, count, element.getLength());
    }

    private void installTypes() throws Exception {
        DataTypeManager manager = currentProgram.getDataTypeManager();
        for (var row : object("structures").entrySet()) {
            JsonObject spec = row.getValue().getAsJsonObject();
            StructureDataType structure = new StructureDataType(new CategoryPath("/LameDuke"),
                row.getKey(), 0, manager);
            structure.setExplicitPackingValue(1);
            for (JsonElement field : spec.getAsJsonArray("fields")) {
                JsonArray parts = field.getAsJsonArray();
                DataType fieldType = arrayType(type(parts.get(1).getAsString()), parts.get(2).getAsInt());
                structure.add(fieldType, fieldType.getLength(), parts.get(0).getAsString(), null);
            }
            int expected = spec.get("size").getAsInt();
            if (structure.getLength() != expected) {
                throw new IllegalStateException(structure.getName() + ": expected " + expected +
                    " bytes, got " + structure.getLength());
            }
            structureTypes.put(row.getKey(), manager.resolve(structure, DataTypeConflictHandler.DEFAULT_HANDLER));
        }
        for (JsonElement row : array("types")) {
            JsonArray spec = row.getAsJsonArray();
            Address address = addr(spec.get(0).getAsInt());
            DataType dataType = arrayType(type(spec.get(2).getAsString()), spec.get(3).getAsInt());
            currentProgram.getListing().createData(address, dataType);
        }
        println("Installed " + structureTypes.size() + " packed structures and " + array("types").size() +
            " bound data types");
    }

    private void markJumpTables() throws Exception {
        Listing listing = currentProgram.getListing();
        PointerDataType pointer = new PointerDataType(currentProgram.getDataTypeManager());
        TreeSet<Address> starts = new TreeSet<>();
        for (JsonElement row : array("functions")) {
            starts.add(entry(row.getAsJsonObject()));
        }
        int marked = 0;
        for (JsonElement item : array("jump_table")) {
            Address site = addr(item.getAsInt());
            // funcmap's adjacency heuristic also catches some instruction
            // fields. Skip any dword that crosses an independently named start.
            Address next = starts.ceiling(site);
            if (next == null || next.compareTo(site.add(3)) > 0) {
                listing.createData(site, pointer);
                marked++;
            }
        }
        println("Marked " + marked + " jump-table pointer candidates");
    }

    private AddressSet extent(JsonObject seed) {
        return new AddressSet(entry(seed), addr(seed.get("limit").getAsInt() - 1));
    }

    private void seedFunctions() throws Exception {
        Listing listing = currentProgram.getListing();
        FunctionManager functions = currentProgram.getFunctionManager();
        List<JsonObject> seeds = new ArrayList<>();
        TreeSet<Address> supported = new TreeSet<>();
        for (JsonElement row : array("functions")) {
            JsonObject seed = row.getAsJsonObject();
            seeds.add(seed);
            if (seed.get("priority").getAsInt() < 2 && !seed.get("code_data").getAsBoolean()) {
                supported.add(entry(seed));
            }
        }
        seeds.sort(Comparator.comparingInt(seed -> seed.get("priority").getAsInt()));
        for (JsonObject seed : seeds) {
            monitor.checkCancelled();
            Address start = entry(seed);
            if (seed.get("code_data").getAsBoolean() || listing.getCodeUnitContaining(start)
                    instanceof Data data && data.isDefined()) {
                continue;
            }
            AddressSet bounds = extent(seed);
            if (seed.get("priority").getAsInt() < 2) {
                Address next = supported.higher(start);
                bounds = new AddressSet(start, next == null ? addr(codeEnd - 1) : next.subtract(1));
            }
            if (listing.getInstructionAt(start) == null) {
                new DisassembleCommand(start, bounds, true).applyTo(currentProgram, monitor);
            }
        }
        TreeSet<Address> decoded = new TreeSet<>();
        for (JsonObject seed : seeds) {
            Address start = entry(seed);
            if (listing.getInstructionAt(start) != null) {
                decoded.add(start);
            }
        }
        for (JsonObject seed : seeds) {
            monitor.checkCancelled();
            Address start = entry(seed);
            if (!decoded.contains(start)) {
                continue;
            }
            Address next = decoded.higher(start);
            AddressSet bounds = new AddressSet(start, next == null ? addr(codeEnd - 1) : next.subtract(1));
            AddressSet body = new AddressSet(start);
            body.add(CreateFunctionCmd.getFunctionBody(currentProgram, start, monitor).intersect(bounds));
            Function function = functions.createFunction(seed.get("name").getAsString(), start, body,
                seed.get("name").getAsString().startsWith("FUN_") ? SourceType.DEFAULT : IMPORTED);
            setConvention(function, seed);
        }
        println("Created " + decoded.size() + " seeded functions; " + (seeds.size() - decoded.size()) +
            " candidates have no instruction at their entry");
    }

    private void setConvention(Function function, JsonObject seed) throws Exception {
        String convention = seed.get("convention").getAsString();
        if (!convention.equals("unknown")) {
            function.setCallingConvention(convention);
        }
    }

    private void attachReferences() {
        Listing listing = currentProgram.getListing();
        ReferenceManager manager = currentProgram.getReferenceManager();
        for (var fixup : fixups.entrySet()) {
            Address site = fixup.getKey();
            Address target = fixup.getValue();
            Instruction insn = listing.getInstructionContaining(site);
            Address source = insn == null ? site : insn.getAddress();
            if (Arrays.stream(manager.getReferencesFrom(source)).anyMatch(r -> target.equals(r.getToAddress()))) {
                continue;
            }
            int operand = Reference.MNEMONIC;
            if (insn != null) {
                for (int index = 0; index < insn.getNumOperands(); index++) {
                    for (Object part : insn.getOpObjects(index)) {
                        if (target.equals(part) ||
                                (part instanceof Scalar scalar && scalar.getUnsignedValue() == target.getOffset())) {
                            operand = index;
                        }
                    }
                }
            }
            manager.addMemoryReference(source, target, RefType.DATA, IMPORTED, operand);
        }
    }

    private void applySignatures() throws Exception {
        DataTypeManager manager = currentProgram.getDataTypeManager();
        for (JsonElement row : array("signatures")) {
            JsonArray spec = row.getAsJsonArray();
            Function function = getFunctionAt(addr(spec.get(0).getAsInt()));
            if (function == null) {
                throw new IllegalStateException("Missing typed function " + spec.get(1).getAsString());
            }
            Variable ret = new ReturnParameterImpl(type(spec.get(2).getAsString()).clone(manager), currentProgram);
            List<Parameter> params = new ArrayList<>();
            for (JsonElement item : spec.get(3).getAsJsonArray()) {
                JsonArray arg = item.getAsJsonArray();
                params.add(new ParameterImpl(arg.get(0).getAsString(), type(arg.get(1).getAsString()).clone(manager),
                    currentProgram));
            }
            boolean stackVarargs = spec.get(4).getAsBoolean();
            function.updateFunction(stackVarargs ? "__cdecl" : "__watcall", ret, params,
                Function.FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS,
                true, USER);
            function.setVarArgs(stackVarargs);
        }
    }

    private void export() throws Exception {
        Listing listing = currentProgram.getListing();
        for (String name : object("structures").keySet()) {
            structureTypes.put(name, currentProgram.getDataTypeManager().getDataType("/LameDuke/" + name));
        }
        for (var row : object("names").entrySet()) {
            Address address = addr(Long.parseLong(row.getKey()));
            for (JsonElement alias : row.getValue().getAsJsonArray()) {
                JsonArray fields = alias.getAsJsonArray();
                if (fields.get(2).getAsString().equals("data")) {
                    globalNames.put(address, fields.get(0).getAsString());
                    break;
                }
            }
        }
        // Make sure a future run records any relocation whose source is in
        // instructions recovered after the pre-script (e.g. switch cases).
        attachReferences();
        NavigableMap<Address, JsonObject> seeds = exportSeeds();
        Path out = Files.createDirectories(work.resolve("out"));
        Path samples = Files.createDirectories(work.resolve("samples"));
        DecompInterface decompiler = new DecompInterface();
        DecompileOptions options = new DecompileOptions();
        options.grabFromProgram(currentProgram);
        decompiler.setOptions(options);
        decompiler.toggleCCode(true);
        decompiler.toggleSyntaxTree(false);
        if (!decompiler.openProgram(currentProgram)) {
            throw new IllegalStateException(decompiler.getLastMessage());
        }
        List<Map<String, Object>> index = new ArrayList<>();
        Map<String, Integer> counts = new TreeMap<>();
        try {
            exportControls(decompiler, samples);
            applySignatures();
            decompiler.closeProgram();
            if (!decompiler.openProgram(currentProgram)) {
                throw new IllegalStateException(decompiler.getLastMessage());
            }
            for (JsonObject seed : seeds.values()) {
                monitor.checkCancelled();
                Address address = entry(seed);
                Function function = getFunctionAt(address);
                CodeUnit containing = listing.getCodeUnitContaining(address);
                String name = seed.get("name").getAsString();
                String tu = seed.get("TU").getAsString();
                String coordinate = seed.get("addr").getAsString();
                String relative = tu + "/" + coordinate.substring(2) + "_" + safeName(name) + ".c";
                String status = "decompiled";
                String error = "";
                String code = "";
                if (function == null || listing.getInstructionAt(address) == null) {
                    if (containing instanceof Data data && data.isDefined()) {
                        status = "data";
                        error = "Seed points into defined " + data.getDataType().getDisplayName() +
                            " at " + coordinate(data.getMinAddress()) + ".";
                    }
                    else if (containing instanceof Instruction instruction) {
                        boolean offcut = !address.equals(instruction.getAddress());
                        status = offcut ? "offcut" : "missing_function";
                        error = offcut ? "Seed points inside an instruction at " +
                            coordinate(instruction.getAddress()) + "." : "Analysis left this instruction unbound.";
                    }
                    else {
                        status = "undecoded";
                        error = "No decoded instruction at the seeded entry.";
                    }
                }
                else {
                    DecompileResults result = decompiler.decompileFunction(function, 30, monitor);
                    error = result.getErrorMessage();
                    if (result.decompileCompleted() && result.getDecompiledFunction() != null) {
                        code = result.getDecompiledFunction().getC();
                    }
                    else {
                        status = result.isTimedOut() ? "timeout" : "failed";
                    }
                }
                String convention = function == null ? "unknown" : function.getCallingConventionName();
                String header = "/* Ghidra analysis draft | " + coordinate + " | " + tu +
                    " | " + convention + " | " + status + "\n";
                if (error != null && !error.isBlank()) {
                    header += error.replace("*/", "* /").strip() + "\n";
                }
                Path output = out.resolve(relative);
                Files.createDirectories(output.getParent());
                Files.writeString(output, header + "*/\n" + code);
                Map<String, Object> item = new LinkedHashMap<>();
                for (String key : List.of("addr", "name", "size", "TU", "funcmap", "evidence")) {
                    item.put(key, seed.get(key));
                }
                item.put("file", relative);
                item.put("status", status);
                item.put("convention", convention);
                item.put("body_bytes", function == null ? 0 : function.getBody().getNumAddresses());
                item.put("ghidra_function", function != null);
                if (error != null && !error.isBlank()) {
                    item.put("diagnostic", error.strip());
                }
                if (function == null) {
                    item.put("callees", List.of());
                    item.put("globals", List.of());
                }
                else {
                    addReferences(function, item);
                }
                index.add(item);
                counts.merge(status, 1, Integer::sum);
                if (index.size() % 100 == 0) {
                    println("Exported " + index.size() + " / " + seeds.size());
                }
            }
        }
        finally {
            decompiler.dispose();
        }
        writeJson(out.resolve("index.json"), index);
        writeJson(work.resolve("run.json"), Map.of("counts", counts,
            "relocations", currentProgram.getRelocationTable().getSize(),
            "retained_labels", countLabels(),
            "ghidra_functions", currentProgram.getFunctionManager().getFunctionCount()));
        println("Exported " + index.size() + " entries " + counts);
    }

    private NavigableMap<Address, JsonObject> exportSeeds() throws Exception {
        NavigableMap<Address, JsonObject> seeds = new TreeMap<>();
        NavigableMap<Address, String> regions = new TreeMap<>();
        for (JsonElement row : array("regions")) {
            JsonArray region = row.getAsJsonArray();
            regions.put(addr(region.get(0).getAsInt()), region.get(1).getAsString());
        }
        for (JsonElement row : array("functions")) {
            JsonObject seed = row.getAsJsonObject();
            seeds.put(entry(seed), seed);
        }
        FunctionIterator functions = currentProgram.getFunctionManager().getFunctions(true);
        while (functions.hasNext()) {
            Function function = functions.next();
            Address address = function.getEntryPoint();
            if (seeds.containsKey(address)) {
                continue;
            }
            String tu = regions.floorEntry(address).getValue();
            if ("unknown".equals(function.getCallingConventionName())) {
                switch (tu) {
                    case "creative" -> function.setCallingConvention("__cdecl");
                    case "gravis", "a_asm", "cstart" -> { }
                    default -> function.setCallingConvention("__watcall");
                }
            }
            JsonObject seed = new JsonObject();
            seed.addProperty("linear", address.getOffset());
            seed.addProperty("addr", coordinate(address));
            seed.addProperty("name", function.getName());
            seed.addProperty("TU", tu);
            seed.addProperty("size", function.getBody().getMaxAddress().subtract(address) + 1);
            seed.addProperty("funcmap", false);
            JsonArray evidence = new JsonArray();
            evidence.add("ghidra_autoanalysis");
            seed.add("evidence", evidence);
            seeds.put(address, seed);
        }
        println("Added " + (seeds.size() - array("functions").size()) + " Ghidra-discovered functions");
        return seeds;
    }

    private int countLabels() {
        SymbolTable table = currentProgram.getSymbolTable();
        Set<String> bound = new HashSet<>();
        for (var row : object("names").entrySet()) {
            Address address = addr(Long.parseLong(row.getKey()));
            for (JsonElement alias : row.getValue().getAsJsonArray()) {
                String name = alias.getAsJsonArray().get(0).getAsString();
                if (table.getSymbol(name, address, currentProgram.getGlobalNamespace()) != null) {
                    bound.add(address + ":" + name);
                }
                else {
                    println("Missing original label " + name + " at " + coordinate(address));
                }
            }
        }
        return bound.size();
    }

    private void exportControls(DecompInterface decompiler, Path samples) throws Exception {
        for (var row : object("samples").entrySet()) {
            Function function = getFunctionAt(addr(Long.parseLong(row.getKey())));
            String convention = function.getCallingConventionName();
            for (String mode : List.of("__cdecl", convention)) {
                function.setCallingConvention(mode);
                decompiler.flushCache();
                DecompileResults result = decompiler.decompileFunction(function, 30, monitor);
                String code = result.getDecompiledFunction() == null
                    ? result.getErrorMessage() : result.getDecompiledFunction().getC();
                Files.writeString(samples.resolve(row.getValue().getAsString() + mode + ".c"), code);
            }
        }
    }

    private String safeName(String name) {
        return name.replaceAll("[^A-Za-z0-9_.$-]", "_");
    }

    private void addReferences(Function function, Map<String, Object> item) {
        TreeMap<Address, String> callees = new TreeMap<>();
        TreeMap<Address, String> globals = new TreeMap<>();
        Listing listing = currentProgram.getListing();
        ReferenceManager references = currentProgram.getReferenceManager();
        InstructionIterator iter = listing.getInstructions(function.getBody(), true);
        while (iter.hasNext()) {
            Instruction instruction = iter.next();
            for (Reference ref : references.getReferencesFrom(instruction.getAddress())) {
                Address target = ref.getToAddress();
                Function callee = getFunctionAt(target);
                RefType refType = ref.getReferenceType();
                if (callee != null && (refType.isCall() ||
                        (refType.isJump() && !function.equals(callee) && !refType.isComputed()))) {
                    callees.put(target, callee.getName());
                }
                addGlobal(target, globals);
            }
            for (Address target : fixups.subMap(instruction.getMinAddress(), true,
                    instruction.getMaxAddress(), true).values()) {
                addGlobal(target, globals);
            }
        }
        item.put("callees", referenceList(callees));
        item.put("globals", referenceList(globals));
    }

    private void addGlobal(Address target, Map<Address, String> globals) {
        if (target.getAddressSpace().equals(addr(dataStart).getAddressSpace()) &&
                target.getOffset() >= dataStart && target.getOffset() < dataEnd) {
            Data data = currentProgram.getListing().getDefinedDataContaining(target);
            String name = globalNames.get(target);
            if (name == null && data != null) {
                Address base = data.getMinAddress();
                String baseName = globalNames.get(base);
                if (baseName != null) {
                    name = baseName + "+0x" + Long.toHexString(target.subtract(base));
                }
            }
            if (name == null) {
                Symbol symbol = currentProgram.getSymbolTable().getPrimarySymbol(target);
                name = symbol == null ? "DAT_" + target : symbol.getName();
            }
            globals.put(target, name);
        }
    }

    private List<Map<String, String>> referenceList(Map<Address, String> refs) {
        return refs.entrySet().stream().map(ref ->
            Map.of("addr", coordinate(ref.getKey()), "name", ref.getValue())).toList();
    }

    private void writeJson(Path path, Object value) throws IOException {
        try (Writer writer = Files.newBufferedWriter(path)) {
            JSON.toJson(value, writer);
            writer.write("\n");
        }
    }
}
