// import React from './react/react.development';
// import ReactDOM from './react/react-dom.development';
// // import './index.css';
// // import App from './App';
// // import reportWebVitals from './reportWebVitals';

// // import './react/react.development';
// // import './react/react-dom.development';

// const root = ReactDOM.createRoot(document.getElementById('root'));
// // root.render(
// //   <div>Hello </div>
// //   // <React.StrictMode>
// //     // <App />
// //   // </React.StrictMode>
// // );

// console.log(root);
// // If you want to start measuring performance in your app, pass a function
// // to log results (for example: reportWebVitals(console.log))
// // or send to an analytics endpoint. Learn more: https://bit.ly/CRA-vitals
// reportWebVitals();

'use strict';

const e = React.createElement;

class LikeButton extends React.Component {
  constructor(props) {
    super(props);
    this.state = { liked: false };
  }

  render() {
    if (this.state.liked) {
      return 'You liked comment number ' + this.props.commentID;
    }

    return e(
      'button',
      { onClick: () => this.setState({ liked: true }) },
      'Like'
    );
  }
}

// Find all DOM containers, and render Like buttons into them.
document.querySelectorAll('.like_button_container')
  .forEach(domContainer => {
    // Read the comment ID from a data-* attribute.
    const commentID = parseInt(domContainer.dataset.commentid, 10);
    const root = ReactDOM.createRoot(domContainer);
    root.render(
      e(LikeButton, { commentID: commentID })
    );
  });