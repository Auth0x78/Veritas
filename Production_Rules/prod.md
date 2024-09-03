$$
\begin {align}
\text{program} \to 
\begin {cases}
    \text{GlobalDecl Functions}
\end {cases} \\

\text{GlobalDecls} \to 
\begin {cases}
    \text{DeclStmt GlobalDecls} \\
    \epsilon
\end {cases} \\

\text{Functions} \to 
\begin {cases}
    \text{FunctionDefn} \\
    \epsilon
\end {cases} \\

\text{FunctionDefn} \to 
\begin {cases}
    \text{`fn' ident} \to \text{Type `(' ParamDecl* `)'} \text{ CompoundStmt} \\
    \text{`fn' fn-linkage-specifier? ident} \to \text{Type `(' ParamDecl* `)' `;'}
\end {cases} \\

\text{ParamDecl} \to 
\begin{cases} 
    \text{ident `:' Type} \\
    \text{`...'}\\
\end{cases} \\

\text {fn-linkage-specifier} \to \text{'extern'} \\

\text{DeclStmt} \to 
\begin {cases}
    \text{'let' ident ':' Type '=' Expr} \\
    \text{'let' ident ':' Type ';'} \\
\end {cases} \\

\text{Type} \to 
\begin {cases}
    \text{ident} \\
    \text{BuiltinType} \\
\end {cases} \\

\text{BuiltinType} \to 
\begin {cases}
    \text{'i8' |\ `i16' |\ `i32' |\ `i64' |\ `i128'}\\
    \text{`u8' |\ 'u16' |\ `u32' |\ `u64' |\ `u128'}\\
    \text{`f32'|\ `f64'} \\
    \text{`nchar'|\ 'wchar'}
\end {cases} \\

\text {CompoundStmt} \to 
\begin {cases}
    \text{`\{' [Statment]* `\}' }
\end {cases} \\

\text {Statement} \to 
\begin {cases}
    \text {DeclStmt} \\
    \text {AssignStmt} \\
    \text {FnCallStmt}\ _{note:\ Discard\ return\ value} \\
    \text {CompoundStmt} \\
    \text {ReturnStmt}
\end {cases} \\
\text {FnCallStmt} \to \text{ident`(' ParamList `)' `;'} \\
\text {AssignStmt} \to \text{ident `=' Expr} \\

\text {ReturnStmt} \to \text{`return' Expr}  \\

\text{Expr} \to
\begin{cases}
    \text{Expr `;'} \\
    \text{`(' Expr `)'}\\
    \text{Op Expr} \\
    \text{Expr ident `(' ParamList `)' Expr} \\
    \text{ident} \\
    \text{literal} \\
\end {cases} \\

\text{Op} \to
\begin{cases}
    \text{15, Left-Right}
        \begin{cases}
            ()\ FnCall \\
            [] \\
            . \\
            \to \\
        \end{cases}    \\
        
    \text{14, Right-Left}
        \begin{cases}
            +, -\ (Unary) \\
            !\\
            tilde\ \tilde \\\\
            (Type)\ or\ Cast \\
            * (dereference)\\
            \& \\
        \end{cases}    \\
    \text{13, Left-Right}
        \begin{cases}
            \text{\^ } \ Precedence = 13.5 \\
            *\\
            /\\
            \%\\
        \end{cases}    \\
        
    \text{12, Left-Right}
        \begin{cases}
            +\\
            -\\
        \end{cases}    \\
        
    \text{11, Left-Right}
        \begin{cases}
            <<\\
            >>\\
        \end{cases}    \\

    \text{10, Left-Right}
        \begin{cases}
            <\\
            <=\\
            >\\
            >=\\
        \end{cases}    \\
        
    \text{9, Left-Right}
        \begin{cases}
            ==\\
            !=\\
        \end{cases}    \\
    \text{8, Left-Right}
        \begin{cases}
            \& \\
        \end{cases}    \\
        
    \text{7, Left-Right}
        \begin{cases}
            \text{BITWISE XOR \^ }\\
        \end{cases}    \\
        
    \text{6, Left-Right}
        \begin{cases}
            |\\
        \end{cases}    \\
        
    \text{5, Left-Right}
        \begin{cases}
            \&\&\\
        \end{cases}    \\
    \text{4, Left-Right}
        \begin{cases}
            ||\\
        \end{cases}    \\
    \text{TODO: Add left symbols}\\
    \text{1, Left-Right}
        \begin{cases}
            , Comma\\
        \end{cases}    \\
\end {cases}


\end{align}
$$