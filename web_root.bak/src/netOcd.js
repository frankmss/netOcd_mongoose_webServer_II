'use strict';
// import { render, Component,h} from '../preact.min.js';


// class App extends Component {
//   // render(){
//   //   return(
//   //     <div>
//   //       <h1> hello preact</h1>
//   //     </div>
//   //   );
//   // }
// }

// window.onload = () => render(h(App), document.body);

// import { h, Component, render } from 'https://unpkg.com/preact?module';
// import htm from 'https://unpkg.com/htm?module';


//****ok */
// import {h, Component, render, html} from '../preact.min.js'
import { render } from "./preact.js";

function App (props) {
  return html`<h1>Hello ${props.name}!</h1>`;
}

render(html`<${App} name="World" />`, document.body);
//****ok */

// import {h, Component, render, html} from '../preact.min.js'
// import { h, render, Component } from 'preact';

// class App extends Component {
//   render() {
//     return <h1>Hello, world!</h1>;
//   }
// }

// render(<App />, document.getElementById("app"));