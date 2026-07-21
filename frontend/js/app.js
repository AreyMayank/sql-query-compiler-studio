document.addEventListener('DOMContentLoaded', () => {
    // DOM Elements
    const sqlEditor = document.getElementById('sql-editor');
    const highlightOverlay = document.getElementById('highlight-overlay');
    const lineNumbers = document.getElementById('line-numbers');
    const btnRun = document.getElementById('btn-run');
    const btnClear = document.getElementById('btn-clear');
    const presetSelect = document.getElementById('preset-queries');
    const schemaTree = document.getElementById('schema-tree');
    const schemaJsonEditor = document.getElementById('schema-json-editor');
    const btnSaveSchema = document.getElementById('btn-save-schema');
    const compileBadge = document.getElementById('compile-badge');
    const engineBadge = document.getElementById('engine-badge');
    const tokenFilter = document.getElementById('token-filter');

    let currentCompilerData = null;

    // Live SQL Syntax Highlighting function
    function highlightSQL(text) {
        if (!text) return '';
        
        const tokenPatterns = [
            { type: 'keyword', regex: /^(SELECT|FROM|WHERE|ORDER\s+BY|LIMIT|ASC|DESC|AND|OR|NOT|BY)\b/i },
            { type: 'string', regex: /^'([^'\\]|\\.)*'/ },
            { type: 'number', regex: /^\d+(\.\d+)?\b/ },
            { type: 'operator', regex: /^(<=|>=|!=|<>|=|<|>)/ },
            { type: 'identifier', regex: /^[a-zA-Z_][a-zA-Z0-9_]*/ },
            { type: 'whitespace', regex: /^\s+/ },
            { type: 'other', regex: /^./ }
        ];

        let html = '';
        let pos = 0;
        while (pos < text.length) {
            let match = null;
            for (const pat of tokenPatterns) {
                const m = pat.regex.exec(text.slice(pos));
                if (m) {
                    match = { type: pat.type, val: m[0] };
                    break;
                }
            }
            if (match) {
                let valEscaped = match.val
                    .replace(/&/g, '&amp;')
                    .replace(/</g, '&lt;')
                    .replace(/>/g, '&gt;');
                
                if (match.type === 'keyword') {
                    html += `<span class="hl-keyword">${valEscaped}</span>`;
                } else if (match.type === 'string') {
                    html += `<span class="hl-string">${valEscaped}</span>`;
                } else if (match.type === 'number') {
                    html += `<span class="hl-number">${valEscaped}</span>`;
                } else if (match.type === 'operator') {
                    html += `<span class="hl-operator">${valEscaped}</span>`;
                } else {
                    html += valEscaped;
                }
                pos += match.val.length;
            } else {
                html += text[pos];
                pos++;
            }
        }
        return html;
    }

    // Line numbers & highlights sync
    function syncEditor() {
        highlightOverlay.innerHTML = highlightSQL(sqlEditor.value) + '\n';
        const lines = sqlEditor.value.split('\n').length;
        lineNumbers.innerHTML = Array.from({ length: lines }, (_, i) => i + 1).join('<br>');
    }

    sqlEditor.addEventListener('input', syncEditor);
    sqlEditor.addEventListener('scroll', () => {
        highlightOverlay.scrollTop = sqlEditor.scrollTop;
        highlightOverlay.scrollLeft = sqlEditor.scrollLeft;
        lineNumbers.scrollTop = sqlEditor.scrollTop;
    });

    // Tab switching in schema card
    document.querySelectorAll('.tabs .tab-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            document.querySelectorAll('.tabs .tab-btn').forEach(b => b.classList.remove('active'));
            document.querySelectorAll('.schema-card .tab-content').forEach(c => c.classList.remove('active'));
            btn.classList.add('active');
            document.getElementById(btn.dataset.tab).classList.add('active');
        });
    });

    // Stepper navigation
    const stepNodes = document.querySelectorAll('.step-node');
    const stageViews = document.querySelectorAll('.stage-view');

    stepNodes.forEach(node => {
        node.addEventListener('click', () => {
            const targetViewId = node.dataset.view;
            stepNodes.forEach(n => n.classList.remove('active'));
            stageViews.forEach(v => v.classList.remove('active'));
            
            node.classList.add('active');
            document.getElementById(targetViewId).classList.add('active');
        });
    });

    // Fetch sample queries
    async function loadSampleQueries() {
        try {
            const res = await fetch('/api/samples');
            if (!res.ok) return;
            const samples = await res.json();
            presetSelect.innerHTML = '<option value="">-- Load Sample Query --</option>';
            samples.forEach((sample, idx) => {
                const opt = document.createElement('option');
                opt.value = sample.query;
                opt.textContent = sample.title;
                presetSelect.appendChild(opt);
            });
        } catch (e) {
            console.error('Error loading samples:', e);
        }
    }

    presetSelect.addEventListener('change', (e) => {
        if (e.target.value) {
            sqlEditor.value = e.target.value;
            syncEditor();
        }
    });

    // Fetch schema and render including Sample Data
    async function loadSchema() {
        try {
            const res = await fetch('/api/schema');
            if (!res.ok) return;
            const schema = await res.json();
            schemaJsonEditor.value = JSON.stringify(schema, null, 2);

            schemaTree.innerHTML = '';
            const tables = schema.tables || {};
            for (const [tblName, tblObj] of Object.entries(tables)) {
                const node = document.createElement('div');
                node.className = 'schema-table-node';
                
                let colsHtml = '';
                const cols = tblObj.columns || {};
                for (const [cName, cType] of Object.entries(cols)) {
                    colsHtml += `
                        <div class="schema-col">
                            <span class="schema-col-name"><i class="fa-solid fa-cube"></i> ${cName}</span>
                            <span class="schema-col-type">${cType}</span>
                        </div>
                    `;
                }

                // Render small sample data records
                let dataHtml = '';
                const sampleRows = tblObj.data || [];
                if (sampleRows.length > 0) {
                    const colNames = Object.keys(cols);
                    let ths = colNames.map(name => `<th>${name}</th>`).join('');
                    let trs = sampleRows.map(row => {
                        let tds = colNames.map(name => `<td>${row[name] !== undefined ? row[name] : ''}</td>`).join('');
                        return `<tr>${tds}</tr>`;
                    }).join('');
                    
                    dataHtml = `
                        <div class="schema-data-wrapper">
                            <h6>Sample Data</h6>
                            <table class="schema-data-table">
                                <thead><tr>${ths}</tr></thead>
                                <tbody>${trs}</tbody>
                            </table>
                        </div>
                    `;
                }

                node.innerHTML = `
                    <div class="schema-table-header" onclick="this.nextElementSibling.style.display = this.nextElementSibling.style.display === 'none' ? 'block' : 'none'">
                        <span><i class="fa-solid fa-table"></i> ${tblName}</span>
                        <span class="muted">${Object.keys(cols).length} cols (click to toggle)</span>
                    </div>
                    <div class="schema-table-columns" style="display: block;">
                        ${colsHtml}
                        ${dataHtml}
                    </div>
                `;
                schemaTree.appendChild(node);
            }
        } catch (e) {
            console.error('Error loading schema:', e);
        }
    }

    btnSaveSchema.addEventListener('click', async () => {
        try {
            const parsed = JSON.parse(schemaJsonEditor.value);
            const res = await fetch('/api/schema', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(parsed)
            });
            if (res.ok) {
                alert('Schema updated successfully!');
                loadSchema();
            }
        } catch (e) {
            alert('Invalid JSON schema syntax!');
        }
    });

    // Clear editor
    btnClear.addEventListener('click', () => {
        sqlEditor.value = '';
        presetSelect.value = '';
        syncEditor();
    });

    // Execute compilation
    btnRun.addEventListener('click', async () => {
        const query = sqlEditor.value.strip ? sqlEditor.value.strip() : sqlEditor.value.trim();
        if (!query) {
            alert('Please enter a SQL query to compile.');
            return;
        }

        btnRun.disabled = true;
        btnRun.innerHTML = '<i class="fa-solid fa-spinner fa-spin"></i> Compiling...';
        compileBadge.querySelector('.badge-text').textContent = 'Status: Compiling...';

        try {
            const res = await fetch('/api/compile', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ query })
            });

            const data = await res.json();
            currentCompilerData = data;

            // Engine badge update
            engineBadge.querySelector('.pulse-indicator').className = 'pulse-indicator';
            engineBadge.querySelector('.badge-text').textContent = `Engine: ${data.engine || 'Active'}`;

            if (data.success) {
                compileBadge.querySelector('.pulse-indicator').className = 'pulse-indicator';
                compileBadge.querySelector('.badge-text').textContent = 'Status: Success';
            } else {
                compileBadge.querySelector('.pulse-indicator').className = 'pulse-indicator red';
                compileBadge.querySelector('.badge-text').textContent = 'Status: Failed';
            }

            renderCompilerResults(data);
        } catch (e) {
            console.error('Compilation error:', e);
            alert('Error calling compiler API.');
        } finally {
            btnRun.disabled = false;
            btnRun.innerHTML = '<i class="fa-solid fa-play"></i> Compile Query';
        }
    });

    // Render step results
    function renderCompilerResults(data) {
        renderTokens(data.tokens || []);
        renderAST(data.ast || {});
        renderSemantic(data.semantic || {});
        renderQueryTree(data.query_tree || {});
        renderOptimizer(data.optimized || {});
        renderExecutionPlan(data.execution_plan || {});

        // Update step status styles
        stepNodes.forEach((node, idx) => {
            node.classList.remove('success', 'error');
            if (data.success) {
                node.classList.add('success');
            } else {
                if (idx <= 2) node.classList.add(data.semantic && data.semantic.success ? 'success' : 'error');
            }
        });
    }

    // 1. Tokens Renderer
    function renderTokens(tokens) {
        const tbody = document.querySelector('#token-table tbody');
        tbody.innerHTML = '';
        if (!tokens || tokens.length === 0) {
            tbody.innerHTML = '<tr><td colspan="5" class="placeholder-row">No tokens generated.</td></tr>';
            return;
        }

        tokens.forEach((tok, idx) => {
            const tr = document.createElement('tr');
            let clsName = 'tok-identifier';
            if (tok.type === 'KEYWORD') clsName = 'tok-keyword';
            else if (tok.type === 'OPERATOR') clsName = 'tok-operator';
            else if (tok.type.includes('LITERAL')) clsName = 'tok-literal';
            else if (tok.type === 'DELIMITER') clsName = 'tok-delimiter';

            tr.innerHTML = `
                <td>${idx + 1}</td>
                <td><span class="${clsName}">${tok.type}</span></td>
                <td><code>${escapeHtml(tok.value)}</code></td>
                <td>${tok.line}</td>
                <td>${tok.column}</td>
            `;
            tbody.appendChild(tr);
        });
    }

    tokenFilter.addEventListener('input', (e) => {
        const val = e.target.value.toLowerCase();
        const rows = document.querySelectorAll('#token-table tbody tr');
        rows.forEach(tr => {
            const text = tr.textContent.toLowerCase();
            tr.style.display = text.includes(val) ? '' : 'none';
        });
    });

    // 2. AST Renderer (Mermaid.js graph)
    function renderAST(astNode) {
        const target = document.getElementById('ast-diagram');
        if (!astNode || !astNode.type) {
            target.innerHTML = '<p class="muted">No AST generated due to syntax error.</p>';
            return;
        }

        let nodeCounter = 0;
        let mermaidSrc = 'graph TD\n';

        function buildMermaidAst(node) {
            const currentId = `node_${nodeCounter++}`;
            let label = node.type;
            if (node.value !== undefined) label += `\\nval: ${node.value}`;
            if (node.op) label += `\\nop: ${node.op}`;

            mermaidSrc += `  ${currentId}["${escapeMermaid(label)}"]\n`;

            if (node.children) {
                node.children.forEach(child => {
                    if (child) {
                        const childId = buildMermaidAst(child);
                        mermaidSrc += `  ${currentId} --> ${childId}\n`;
                    }
                });
            }
            return currentId;
        }

        buildMermaidAst(astNode);
        renderMermaidGraph(target, mermaidSrc, 'ast_svg');
    }

    // 3. Semantic Renderer
    function renderSemantic(semanticData) {
        const statusCard = document.getElementById('semantic-status-card');
        const statusTitle = document.getElementById('semantic-status-title');
        const statusDesc = document.getElementById('semantic-status-desc');
        const resolvedSt = document.getElementById('resolved-symbol-table');
        const errorsList = document.getElementById('semantic-errors');
        const warningsList = document.getElementById('semantic-warnings');

        errorsList.innerHTML = '';
        warningsList.innerHTML = '';

        if (semanticData.success) {
            statusCard.className = 'semantic-status-card success';
            statusCard.querySelector('.status-icon').innerHTML = '<i class="fa-solid fa-circle-check"></i>';
            statusTitle.textContent = 'Semantic Validation Passed';
            statusDesc.textContent = `Target table '${semanticData.resolved_table || ''}' and all referenced columns exist in schema with compatible data types.`;
        } else {
            statusCard.className = 'semantic-status-card error';
            statusCard.querySelector('.status-icon').innerHTML = '<i class="fa-solid fa-triangle-exclamation"></i>';
            statusTitle.textContent = 'Semantic Check Failed';
            statusDesc.textContent = semanticData.error || 'Queries contain semantic errors (invalid column, table, or type mismatch).';
        }

        // Errors
        const errors = semanticData.errors || [];
        if (errors.length === 0) {
            errorsList.innerHTML = '<li class="muted">None</li>';
        } else {
            errors.forEach(err => {
                const li = document.createElement('li');
                li.textContent = err;
                errorsList.appendChild(li);
            });
        }

        // Warnings
        const warnings = semanticData.warnings || [];
        if (warnings.length === 0) {
            warningsList.innerHTML = '<li class="muted">None</li>';
        } else {
            warnings.forEach(warn => {
                const li = document.createElement('li');
                li.textContent = warn;
                warningsList.appendChild(li);
            });
        }

        // Symbol Table Info
        const st = semanticData.symbol_table;
        if (st && st.columns) {
            let html = `<p><strong>Table:</strong> ${st.table}</p><div style="margin-top:10px;">`;
            st.columns.forEach(c => {
                html += `
                    <div class="sym-col-row">
                        <span>${c.name}</span>
                        <span class="schema-col-type">${c.type}</span>
                    </div>
                `;
            });
            html += '</div>';
            resolvedSt.innerHTML = html;
        } else {
            resolvedSt.innerHTML = '<p class="muted">No symbols resolved.</p>';
        }
    }

    // 4. Query Tree Renderer
    function renderQueryTree(qtNode) {
        const target = document.getElementById('query-tree-diagram');
        if (!qtNode || !qtNode.type) {
            target.innerHTML = '<p class="muted">No Query Tree generated.</p>';
            return;
        }

        let nodeCounter = 0;
        let mermaidSrc = 'graph TD\n';

        function buildMermaidQt(node) {
            const currentId = `qt_${nodeCounter++}`;
            let label = node.type;
            if (node.value) label += `\\n[${node.value}]`;

            mermaidSrc += `  ${currentId}["${escapeMermaid(label)}"]\n`;

            if (node.child) {
                const childId = buildMermaidQt(node.child);
                mermaidSrc += `  ${currentId} --> ${childId}\n`;
            }
            return currentId;
        }

        buildMermaidQt(qtNode);
        renderMermaidGraph(target, mermaidSrc, 'qt_svg');
    }

    // 5. Optimizer Renderer
    function renderOptimizer(optData) {
        const rulesList = document.getElementById('opt-rules-list');
        const target = document.getElementById('opt-tree-diagram');

        rulesList.innerHTML = '';
        const rules = optData.rules_applied || [];
        if (rules.length === 0) {
            rulesList.innerHTML = '<li class="muted">No optimization transformations required.</li>';
        } else {
            rules.forEach(rule => {
                const li = document.createElement('li');
                li.textContent = rule;
                rulesList.appendChild(li);
            });
        }

        // Render the optimized tree to the opt target
        const optNode = optData.optimized_tree;
        if (optNode) {
            let nodeCounter = 0;
            let mermaidSrc = 'graph TD\n';
            function buildMermaidOpt(node) {
                const currentId = `opt_${nodeCounter++}`;
                let label = node.type;
                if (node.value) label += `\\n[${node.value}]`;
                mermaidSrc += `  ${currentId}["${escapeMermaid(label)}"]\n`;
                if (node.child) {
                    const childId = buildMermaidOpt(node.child);
                    mermaidSrc += `  ${currentId} --> ${childId}\n`;
                }
                return currentId;
            }
            buildMermaidOpt(optNode);
            renderMermaidGraph(target, mermaidSrc, 'opt_svg');
        } else {
            target.innerHTML = '<p class="muted">No optimized tree.</p>';
        }
    }

    // 6. Execution Plan Renderer
    function renderExecutionPlan(execData) {
        const opsList = document.getElementById('exec-ops-list');
        const bytecodeOutput = document.getElementById('bytecode-output');

        opsList.innerHTML = '';
        const operators = execData.operators || [];
        if (operators.length === 0) {
            opsList.innerHTML = '<p class="muted">No physical execution plan available.</p>';
        } else {
            operators.forEach(op => {
                const div = document.createElement('div');
                div.className = 'exec-op-node';
                div.innerHTML = `
                    <div class="exec-op-details">
                        <div class="exec-step-num">${op.step}</div>
                        <div class="exec-op-info">
                            <h5>${op.op}</h5>
                            <p>${escapeHtml(op.details)}</p>
                        </div>
                    </div>
                    <div class="exec-op-cost">${op.cost}</div>
                `;
                opsList.appendChild(div);
            });
        }

        const bytecode = execData.bytecode || [];
        bytecodeOutput.textContent = bytecode.length > 0 ? bytecode.join('\n') : '-- No bytecode generated --';
    }

    // Helper: render Mermaid diagram safely
    async function renderMermaidGraph(containerElem, code, uniqueId) {
        try {
            if (window.mermaid) {
                const id = `svg_${uniqueId}_${Math.floor(Math.random() * 100000)}`;
                const renderResult = await mermaid.render(id, code);
                const svg = (renderResult && typeof renderResult === 'object') ? renderResult.svg : renderResult;
                containerElem.innerHTML = svg;
            } else {
                containerElem.innerHTML = `<pre style="font-family: monospace; text-align: left; background: rgba(255,255,255,0.03); padding: 12px; border-radius: 8px; font-size: 12px; border: 1px solid rgba(255,255,255,0.05); color: #8a99ad;">${escapeHtml(code)}</pre>`;
            }
        } catch (e) {
            console.error('Mermaid render error:', e);
            containerElem.innerHTML = `<pre style="font-family: monospace; text-align: left; background: rgba(255,255,255,0.03); padding: 12px; border-radius: 8px; font-size: 12px; border: 1px solid rgba(255,255,255,0.05); color: #8a99ad;">${escapeHtml(code)}</pre>`;
        }
    }

    function escapeHtml(str) {
        if (str === null || str === undefined) return '';
        return String(str).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;").replace(/'/g, "&#039;");
    }

    function escapeMermaid(str) {
        if (str === null || str === undefined) return '';
        return String(str).replace(/"/g, "'");
    }

    // Initialize initial state
    loadSampleQueries();
    loadSchema();
    syncEditor();
});
