// Built-in Node.js modules
let fs = require('fs');
let path = require('path');

// NPM modules
let express = require('express');
let sqlite3 = require('sqlite3');
let chart = require('chart.js');
const e = require('express');
const { exit } = require('process');


let public_dir = path.join(__dirname, 'public');
let template_dir = path.join(__dirname, 'templates');
let db_filename = path.join(__dirname, 'db', 'usenergy.sqlite3');


let app = express();
let port = 8000;

app.use("/public", express.static(path.join(__dirname, 'public')));


// Open usenergy.sqlite3 database
let db = new sqlite3.Database(db_filename, sqlite3.OPEN_READONLY, (err) => {
    if (err) {
        console.log('Error opening ' + db_filename);
    }
    else {
        console.log('Now connected to ' + db_filename);
    }
});

// Serve static files from 'public' directory
app.use(express.static(public_dir));


// GET request handler for home page '/' (redirect to /year/2018)
app.get('/', (req, res) => {
    res.redirect('/year/2018');
});

// GET request handler for '/year/*'
app.get('/year/:selected_year', (req, res) => {
    fs.readFile(path.join(template_dir, 'year.html'), 'utf-8', (err, template) => {
        // modify `template` and send response
        // this will require a query to the SQL database
        if (err) {
            res.status(404).send('404 Error not found');
        } else {

            let response = template.replace(/{{{YEAR}}}/g, req.params.selected_year);
            db.all('SELECT * from Consumption WHERE year = ? ORDER BY state_abbreviation', [req.params.selected_year], (err, row) => {

                
                if (err) {
                    res.status(404).send('404 Error not found');
                } else {
                //response = response.replace(/{{{COAL_COUNT}}}/g, row[0].coal); 
                //response = response.replace('{{{NATURAL_GAS_COUNT}}}', row[0].natural_gas);
                //response = response.replace('{{{NUCLEAR_COUNT}}}', row[0].nuclear);
                //response = response.replace('{{{PETROLEUM_COUNT}}}', row[0].petroleum);
                //response = response.replace('{{{RENEWABLE_COUNT}}}', row[0].renewable);
                db.all('SELECT state_name from States', (err, rows) => {
                    if(req.params.selected_year > 2018 || req.params.selected_year < 1960) {
                        res.status(404).type('text').send("Error: the year " + req.params.selected_year + " does not have data");
                        return;
                    }
                    
                    //console.log(rows[0])
                    let i;
                    let list_items= '';
                    let coal_total = 0;
                    let natural_total = 0;
                    let nuclear_total = 0;
                    let petroleum_total = 0;
                    let renewable_total = 0;
                    for (i = 0; i < rows.length; i++) {
                        coal_total = coal_total + parseInt(row[i].coal);
                        natural_total = natural_total + parseInt(row[i].natural_gas);
                        nuclear_total = nuclear_total + parseInt(row[i].nuclear);
                        petroleum_total = petroleum_total + parseInt(row[i].petroleum);
                        renewable_total = renewable_total + parseInt(row[i].renewable);
                        let total = parseInt(row[i].coal) + parseInt(row[i].natural_gas) + parseInt(row[i].nuclear) + parseInt(row[i].petroleum) + parseInt(row[i].renewable);
                        list_items += '<tr><td>' + rows[i].state_name + '</td>\n' + '<td>' + row[i].coal + '</td>\n' + '<td>' + row[i].natural_gas + '</td>\n' + '<td>' + row[i].nuclear + '</td>\n' + '<td>' + row[i].petroleum + '</td>\n' + '<td>' + row[i].renewable + '</td>\n' + '<td>' + total + '</td></tr>\n';
                    }
                    
                    response = response.replace(/{{{COAL_COUNT}}}/g, coal_total); 
                    response = response.replace(/{{{NATURAL_GAS_COUNT}}}/g, natural_total);
                    response = response.replace(/{{{NUCLEAR_COUNT}}}/g, nuclear_total);
                    response = response.replace(/{{{PETROLEUM_COUNT}}}/g, petroleum_total);
                    response = response.replace(/{{{RENEWABLE_COUNT}}}/g, renewable_total);

                    response = response.replace('{{{STATES}}}', list_items);
                    //console.log(response);
                    res.status(200).type('html').send(response); 

                });
    
                }
            });
        }
    });
});

// GET request handler for '/state/*'
app.get('/state/:selected_state', (req, res) => {
    fs.readFile(path.join(template_dir, 'state.html'), (err, template) => {
        // modify `template` and send response
        // this will require a query to the SQL database
        if (err) {
            res.status(404).send('404 Error not found');
        }//if
        else {


            db.all('SELECT * from Consumption WHERE state_abbreviation =? ORDER BY year', [req.params.selected_state.toUpperCase()], (err, rows) => {
                let response = template.toString();
                //console.log(rows[0].coal);

                //let k;
                //for (k = 0; k < 100; k++) {
                //    console.log(rows[k]);
                let state_fullname = '';

                db.all('SELECT state_name from States WHERE state_abbreviation =?', [req.params.selected_state.toUpperCase()], (err, names) => {
                    
                    //state_fullname = names[0].state_name
                    if(err){
                        res.status(404).send('404 Error not found');
                    }
                    if (names.length==0) {
                        res.status(404).send('Error: the state with abbreviation ' + req.params.selected_state.toUpperCase() + ' does not exist');
                        return;
                    }//if
                    
                    else{
                    //}
                    let year_list = [];
                    let total_list = [];
                    let i;
                        let list_items= '';
                        let year = "";
                        let coal_total = [];
                        let natural_total = [];
                        let nuclear_total = [];
                        let petroleum_total = [];
                        let renewable_total = [];
                        for (i = 0; i < rows.length; i++) {
                            year_list.push('' + rows[i].year);
                            coal_total.push(rows[i].coal);
                            natural_total.push(rows[i].natural_gas);
                            nuclear_total.push(rows[i].nuclear);
                            petroleum_total.push(rows[i].petroleum);
                            renewable_total.push(rows[i].renewable);
                            let total = parseInt(rows[i].coal) + parseInt(rows[i].natural_gas) + parseInt(rows[i].nuclear) + parseInt(rows[i].petroleum) + parseInt(rows[i].renewable);
                            total_list.push(total);
                            list_items += '<tr><td>' +  rows[i].year + '</td>\n' + '<td>' + rows[i].coal + '</td>\n' + '<td>' + rows[i].natural_gas + '</td>\n' + '<td>' + rows[i].nuclear + '</td>\n' + '<td>' + rows[i].petroleum + '</td>\n' + '<td>' + rows[i].renewable + '</td>\n' + '<td>' + total + '</td></tr>\n';
                        }



                    response = response.replace(/{{{YEAR}}}/g, year);
                    response = response.replace(/{{{COAL_COUNTS}}}/g, coal_total); 
                    response = response.replace(/{{{NATURAL_GAS_COUNTS}}}/g, natural_total);
                    response = response.replace(/{{{NUCLEAR_COUNTS}}}/g, nuclear_total);
                    response = response.replace(/{{{PETROLEUM_COUNTS}}}/g, petroleum_total);
                    response = response.replace(/{{{RENEWABLE_COUNTS}}}/g, renewable_total);    
                    response = response.replace(/{{{TOTAL}}}/g, total_list);  
                    response = response.replace(/{{{STATE}}}/g, names[0].state_name);
                    response = response.replace(/{{{YEAR_LIST}}}/g, year_list);
                    response = response.replace('{{{YEARS}}}', list_items);
                    response = response.replace(/{{{STATE_FLAG}}}/g, ('/public/imgs/' + (req.params.selected_state.toLowerCase() + '.png')));
                    response = response.replace(/{{{ALT_TEXT}}}/g, (names[0].state_name + ' Flag'));

                    res.status(200).type('html').send(response);

                
                    }
                });

            });
            
        }//else

        // <-- you may need to change this
    });
});

// GET request handler for '/energy/*'
app.get('/energy/:selected_energy_source', (req, res) => {
    fs.readFile(path.join(template_dir, 'energy.html'), (err, template) => {
        // modify `template` and send response
        // this will require a query to the SQL database

        let energy_source = req.params.selected_energy_source;
        
        db.all('SELECT * from Consumption ORDER BY year, state_abbreviation', (err, rows) => { 
         

                db.all('SELECT DISTINCT year from Consumption', (err, years) => {
    
                    let response = template.toString();
    
                    if (rows[0][energy_source] == undefined) {
                        res.status(404).send('Error: selected energy type of ' + energy_source + ' does not exist');
                        return;
                    }//if
                
                
                    db.all('SELECT * from States', (err,row) => {
                        let i;
                        let list_items= '<th>Year</th>';
                        let year_list = [];
                        let year = "";
                        let energy_counts = {};
                        for (let e = 0; e < row.length; e++){
                            energy_counts[row[e]["state_abbreviation"]] = [];
                        }
    
                        for (i = 0; i < row.length; i++) {
    
                            list_items += '<th>' +  row[i].state_abbreviation + '</th>';
    
                        }
    
                        response = response.replace(/{{{HEADER}}}/g, list_items);
                        list_items = '';
    
    
                        for (j = 0; j < years.length; j++) {
    
                            year_list.push('' + years[j].year);
                            list_items += '<tr><td>' + years[j].year + '</td>'
    
                            for (i = 0; i < 51; i++) {
    
                                new_element = rows[(j*51) + i];
                                energy_counts[new_element["state_abbreviation"]].push(new_element[energy_source]);
                                list_items += '<td>' + new_element[energy_source] + '</td>';
                            }
                            list_items += '</tr>';
                        }
            
                        response = response.replace(/{{{HEADER}}}/g, year);  
                        response = response.replace(/{{{YEARS}}}/g, list_items);
                        response = response.replace(/{{{ENERGY_COUNTS}}}/g, JSON.stringify(energy_counts));  
                        response = response.replace(/{{{ENERGY_TYPE}}}/g, energy_source);
                        response = response.replace(/{{{YEAR_LIST}}}/g, year_list);
                        response = response.replace(/{{{ENERGY_PICTURE}}}/g, ('/public/imgs/' + energy_source + '.jpg'));
                        response = response.replace(/{{{ENERGY_ALT}}}/g, ( energy_source + " picture"));


                        res.status(200).type('html').send(response);
                    });
    
    
                });
    
            
            });

        

    });
});

app.listen(port, () => {
    console.log('Now listening on port ' + port);
});



